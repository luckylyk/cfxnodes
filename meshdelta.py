# This is a maya deformer node prototype
# The purpose is to create a deformer able to compute the delta between two
# geometries and apply it in the deformed one.
# The usage is for animation/vfx pipeline based on geometries cached date
# (whatever the format). If animation is updated and the difference has to be
# applied on further geometry stages. For example a cloth cache. It helps to
# avoid to redo the simulation and all the tweaks.
# Node setup example for cloth update:
#                        _____________
# old animation-\       | meshDelta   |
#                \      |-------------|
#                 \--> *|referenceMesh|
#                 /--> *|offsetMesh   |
#                /      |       output|*-------> cloth sim mesh cached with
# new animation-/       |_____________|          the old animation.


import maya.OpenMaya as om
import maya.OpenMayaMPx as ompx
import sys


PLUGINNAME = 'meshDelta'
PLUGIN_NODENAME = 'meshDelta'
# this plugin id is put randomly and not safe.
# if it does create an plugin id clash in your pipe, just change it
PLUGIN_ID = om.MTypeId(0x002126)


class MeshDelta(ompx.MPxDeformerNode):

    def deform(self, mdatablock, geom_it, local_to_world, geo_idx):
        handle = ompx.cvar.MPxGeometryFilter_envelope
        envelope = mdatablock.inputValue(handle).asFloat()

        handle = mdatablock.inputValue(MeshDelta.reference_mesh)
        reference_mesh = handle.asMesh()
        if reference_mesh.isNull():
            return
        reference_mesh_it = om.MItMeshVertex(reference_mesh)

        handle = mdatablock.inputValue(MeshDelta.offset_mesh)
        offset_mesh = handle.asMesh()
        if offset_mesh.isNull():
            return
        offset_mesh_it = om.MItMeshVertex(offset_mesh)

        if offset_mesh_it.count() != reference_mesh_it.count():
            return

        while not geom_it.isDone():
            position = geom_it.position()
            weight = self.weightValue(mdatablock, geo_idx, geom_it.index())
            ref_position = reference_mesh_it.position()
            offset_position = offset_mesh_it.position()
            offset = ((ref_position - offset_position) * envelope) * weight
            geom_it.setPosition(position - offset)
            geom_it.next()
            reference_mesh_it.next()
            offset_mesh_it.next()


def node_initializer():
    typed_fn = om.MFnTypedAttribute()

    MeshDelta.reference_mesh = typed_fn.create(
        'referenceMesh', 'rm', om.MFnData.kMesh)
    MeshDelta.addAttribute(MeshDelta.reference_mesh)

    MeshDelta.offset_mesh = typed_fn.create(
        'offsetMesh', 'om', om.MFnData.kMesh)
    MeshDelta.addAttribute(MeshDelta.offset_mesh)

    MeshDelta.attributeAffects(
        MeshDelta.reference_mesh,
        ompx.cvar.MPxGeometryFilter_outputGeom)
    MeshDelta.attributeAffects(
        MeshDelta.offset_mesh,
        ompx.cvar.MPxGeometryFilter_outputGeom)


def node_creator():
    return MeshDelta()


def initializePlugin(mobject):
    mplugin = ompx.MFnPlugin(mobject, "Autodesk", "4.5", "Any")
    sys.stdout.write('loading {} BLABLA local...\n'.format(PLUGIN_NODENAME))
    try:
        mplugin.registerNode(
            PLUGIN_NODENAME,
            PLUGIN_ID,
            node_creator,
            node_initializer,
            ompx.MPxNode.kDeformerNode)
        sys.stdout.write('\n... {} loaded\n'.format(PLUGIN_NODENAME))
    except:
        raise


def uninitializePlugin(mobject):
    mplugin = ompx.MFnPlugin(mobject)
    sys.stdout.write('unloading {} ...\n'.format(PLUGIN_NODENAME))
    try:
        mplugin.deregisterNode(PLUGIN_ID)
        sys.stdout.write('... {} unloaded\n'.format(PLUGIN_NODENAME))
    except:
        raise