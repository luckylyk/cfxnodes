import sys
import maya.api.OpenMaya as om


PLUGINNAME = 'lbTension'
PLUGIN_NODENAME = 'meshTensionVertexMap'
PLUGIN_ID = om.MTypeId(0x002125)

MESH_ATTRIBUTE_NAME = 'mesh'
MESH_ATTRIBUTE_SHORTNAME = 'msh'
MESH_ORIG_ATTRIBUTE_NAME = 'referenceMesh'
MESH_ORIG_ATTRIBUTE_SHORTNAME = 'refMesh'
OUTPUT_ATTRIBUTE_NAME = 'vertexMapOutput'
OUTPUT_ATTRIBUTE_SHORTNAME = 'vtxOut'
OUTPUTS_ATTRIBUTE_NAME = 'vertexMapOutputs'
OUTPUTS_ATTRIBUTE_SHORTNAME = 'vtxOuts'
TIME_ATTRIBUTE_NAME = 'time'
TIME_ATTRIBUTE_SHORTNAME = 't'


def maya_useNewAPI():
    pass


class MeshTensionVertexMap(om.MPxNode):
    def compute(self, mplug, mdatablock):
        mesh_plug = om.MPlug(mplug.node(), MeshTensionVertexMap.mesh)
        if mesh_plug.isConnected is False:
            return
        handle = mdatablock.inputValue(mesh_plug)
        mesh = handle.asMesh()
        mesh_fn = om.MFnMesh(mesh)
        mesh_it = om.MItMeshEdge(mesh)
        meshorig_plug = om.MPlug(mplug.node(), MeshTensionVertexMap.meshorig)
        if meshorig_plug.isConnected is False:
            return
        handle = mdatablock.inputValue(meshorig_plug)
        meshorig = handle.asMesh()
        meshorig_it = om.MItMeshEdge(meshorig)
        meshorig_fn = om.MFnMesh(meshorig)

        assert mesh_fn.numVertices == meshorig_fn.numVertices
        assert mesh_fn.numEdges == meshorig_fn.numEdges
        assert mesh_fn.numPolygons == meshorig_fn.numPolygons
        assert mesh_fn.numFaceVertices == meshorig_fn.numFaceVertices

        len_per_vtx = [[] for _ in range(mesh_fn.numVertices)]
        ref_len_per_vtx = [[] for _ in range(mesh_fn.numVertices)]
        while not mesh_it.isDone():
            length = mesh_it.length()
            length_ref = meshorig_it.length()
            for i in (0, 1):
                len_per_vtx[mesh_it.vertexId(i)].append(length)
                ref_len_per_vtx[meshorig_it.vertexId(i)].append(length_ref)
            mesh_it.next()
            meshorig_it.next()
        averages = [(sum(v) / len(v)) / 2 for v in len_per_vtx]
        ref_averages = [(sum(v) / len(v)) / 2 for v in ref_len_per_vtx]
        output = [a / b for a, b in zip(averages, ref_averages)]

        result_handle = mdatablock.outputValue(MeshTensionVertexMap.output)
        output_array = om.MFnDoubleArrayData(result_handle.data())
        output_array.set(output)
        result_handle.setClean()


def node_initializer():
    numeric_fn = om.MFnNumericAttribute()
    typed_fn = om.MFnTypedAttribute()
    default_double_array = om.MDoubleArray()
    doublearray_fn = om.MFnDoubleArrayData()
    doublearray_fn.create(default_double_array)

    MeshTensionVertexMap.mesh = typed_fn.create(
        MESH_ATTRIBUTE_NAME,
        MESH_ATTRIBUTE_SHORTNAME,
        om.MFnData.kMesh)
    MeshTensionVertexMap.addAttribute(MeshTensionVertexMap.mesh)

    MeshTensionVertexMap.meshorig = typed_fn.create(
        MESH_ORIG_ATTRIBUTE_NAME,
        MESH_ORIG_ATTRIBUTE_SHORTNAME,
        om.MFnData.kMesh)
    MeshTensionVertexMap.addAttribute(MeshTensionVertexMap.meshorig)

    MeshTensionVertexMap.output = typed_fn.create(
        OUTPUT_ATTRIBUTE_NAME,
        OUTPUT_ATTRIBUTE_SHORTNAME,
        om.MFnData.kDoubleArray,
        doublearray_fn.object())
    MeshTensionVertexMap.addAttribute(MeshTensionVertexMap.output)

    MeshTensionVertexMap.outputs = numeric_fn.create(
        OUTPUTS_ATTRIBUTE_NAME,
        OUTPUTS_ATTRIBUTE_SHORTNAME,
        om.MFnNumericData.kFloat)
    numeric_fn.setArray(True)
    MeshTensionVertexMap.addAttribute(MeshTensionVertexMap.outputs)

    MeshTensionVertexMap.attributeAffects(
        MeshTensionVertexMap.mesh, MeshTensionVertexMap.output)
    MeshTensionVertexMap.attributeAffects(
        MeshTensionVertexMap.meshorig, MeshTensionVertexMap.output)
    MeshTensionVertexMap.attributeAffects(
        MeshTensionVertexMap.mesh, MeshTensionVertexMap.outputs)
    MeshTensionVertexMap.attributeAffects(
        MeshTensionVertexMap.meshorig, MeshTensionVertexMap.outputs)


def node_creator():
    return MeshTensionVertexMap()


def initializePlugin(mobject):
    mplugin = om.MFnPlugin(mobject, "Autodesk", "4.5", "Any")
    sys.stdout.write('loading {} BLABLA local...\n'.format(PLUGIN_NODENAME))
    try:
        mplugin.registerNode(
            PLUGIN_NODENAME, PLUGIN_ID, node_creator, node_initializer)
        sys.stdout.write('\n... {} loaded\n'.format(PLUGIN_NODENAME))
    except:
        raise


def uninitializePlugin(mobject):
    mplugin = om.MFnPlugin(mobject)
    sys.stdout.write('unloading {} ...\n'.format(PLUGIN_NODENAME))
    try:
        mplugin.deregisterNode(PLUGIN_ID)
        sys.stdout.write('... {} unloaded\n'.format(PLUGIN_NODENAME))
    except:
        raise
