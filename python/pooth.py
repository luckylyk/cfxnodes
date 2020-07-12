
import maya.OpenMaya as om
import maya.OpenMayaMPx as ompx
import sys


PLUGINNAME = 'lbPooth'
PLUGIN_NODENAME = 'pooth'
# this plugin id is put randomly and not safe.
# if it does create an plugin id clash in your pipe, just change it
PLUGIN_ID = om.MTypeId(0x002127)


class Pooth(ompx.MPxDeformerNode):

    def __init__(self):
        ompx.MPxDeformerNode.__init__(self)
        self.vertex_neighbours = None
        self.baked_step = None

    def deform(self, mdatablock, geom_it, local_to_world, geo_index):
        inputmesh = find_input_mesh(mdatablock, geo_index)
        fn_inputmesh = om.MFnMEsh(mesh)

        handle = ompx.cvar.MPxGeometryFilter_envelope
        envelope = mdatablock.inputValue(handle).asFloat()

        handle = Pooth.smooth_iteration
        smooth_interation = mdatablock.inputValue(handle).asInt()

        handle = Pooth.smooth_step_radius
        step_radius = mdatablock.inputValue(handle).asInt()
        if self.vertex_neighbours is None or self.baked_step != step_radius:
            self.interation_value = smooth_interation
            self.baked_step = step_radius
            self.vertex_neighbours = build_vertex_neighbours_dict(
                inputmesh, step_radius)

        push = mdatablock.inputValue(Pooth.push_offset).asFloat()

        push_geometry(geom_it, fn_inputmesh, push, envelope)

        reference_mesh = mdatablock.inputValue(Pooth.reference_mesh).asMesh()
        fn_reference_mesh = om.MFnMesh(reference_mesh)
        reference_mesh_valid = is_meshes_match(fn_reference_mesh, fn_inputmesh)

        handle = mdatablock.inputValue(Pooth.strech_sensitivity)


def push_geometry(geom_it, fn_mesh, push, envelope):
    normals = om.MFloatVectorArray()
    fn_mesh.getVertexNormals(True, normals, om.MSpace.kObject)
    while not geom_it.isDone():
        vertex_index = geom_it.index()
        normal = om.MVector(normals[vertex_index])
        position = geom_it.position()
        # weights = self.weightValue(mdatablock, geo_idx, vertex_index)
        new_position = position + (normal * push * envelope)# * weights
        geom_it.setPosition(new_position)
        geom_it.next()
    geom_it.reset()


def node_initializer():
    typed_fn = om.MFnTypedAttribute()
    numeric_fn = om.MFnNumericAttribute()

    Pooth.smooth_iteration = numeric_fn.create(
        'smoothIteration', 'si', om.MFnNumericData.kInt, 1)
    numeric_fn.setMax(5)
    numeric_fn.setMin(1)
    numeric_fn.setKeyable(True)
    Pooth.addAttribute(Pooth.smooth_iteration)

    Pooth.smooth_step_radius = numeric_fn.create(
        'smoothStepRadius', 'ssr', om.MFnNumericData.kInt, 1)
    numeric_fn.setMin(1)
    numeric_fn.setMax(5)
    numeric_fn.setKeyable(True)
    Pooth.addAttribute(Pooth.smooth_step_radius)

    Pooth.push_offset = numeric_fn.create(
        'pushOffset', 'po', om.MFnNumericData.kFloat, -1.0)
    numeric_fn.setKeyable(True)
    Pooth.addAttribute(Pooth.push_offset)

    Pooth.angle_sensitivity = numeric_fn.create(
        'angleSensitivity', 'as', om.MFnNumericData.kFloat, 0)
    numeric_fn.setKeyable(True)
    numeric_fn.setMax(1)
    numeric_fn.setMin(0)
    Pooth.addAttribute(Pooth.angle_sensitivity)

    Pooth.strech_sensitivity = numeric_fn.create(
        'stretchSensitivity', 'ss', om.MFnNumericData.kFloat, 0)
    numeric_fn.setKeyable(True)
    numeric_fn.setMax(1)
    numeric_fn.setMin(0)
    Pooth.addAttribute(Pooth.strech_sensitivity)

    Pooth.reference_mesh = typed_fn.create(
        'referenceMesh', 'rm', om.MFnData.kMesh)
    typed_fn.setStorable(True)
    Pooth.addAttribute(Pooth.reference_mesh)

    Pooth.attributeAffects(
        Pooth.reference_mesh,
        ompx.cvar.MPxGeometryFilter_outputGeom)

    Pooth.attributeAffects(
        Pooth.smooth_iteration,
        ompx.cvar.MPxGeometryFilter_outputGeom)

    Pooth.attributeAffects(
        Pooth.smooth_step_radius,
        ompx.cvar.MPxGeometryFilter_outputGeom)

    Pooth.attributeAffects(
        Pooth.push_offset,
        ompx.cvar.MPxGeometryFilter_outputGeom)

    Pooth.attributeAffects(
        Pooth.angle_sensitivity,
        ompx.cvar.MPxGeometryFilter_outputGeom)

    Pooth.attributeAffects(
        Pooth.strech_sensitivity,
        ompx.cvar.MPxGeometryFilter_outputGeom)


def node_creator():
    return Pooth()


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


def compute_vertex_tension_values(mesh, reference_mesh):
    mesh_fn = om.MFnMesh(mesh)
    mesh_it = om.MItMeshEdge(mesh)

    refmesh_fn = om.MfnMesh(reference_mesh)
    refmesh_it = om.MItMeshEdge(reference_mesh)

    assert is_meshes_match(mesh_fn, refmesh_fn) is True

    len_per_vtx = [[] for _ in range(mesh_fn.numVertices)]
    ref_len_per_vtx = [[] for _ in range(mesh_fn.numVertices)]
    while not mesh_it.isDone():
        length = mesh_it.length()
        length_ref = refmesh_it.length()
        for i in (0, 1):
            len_per_vtx[mesh_it.vertexId(i)].append(length)
            ref_len_per_vtx[refmesh_it.vertexId(i)].append(length_ref)
        mesh_it.next()
        refmesh_it.next()
    averages = [(sum(v) / len(v)) / 2 for v in len_per_vtx]
    ref_averages = [(sum(v) / len(v)) / 2 for v in ref_len_per_vtx]

    return [a / b for a, b in zip(averages, ref_averages)]


def find_input_mesh(mdatablock, geo_index):
    input_attr = ompx.cvar.MPxGeometryFilter_input
    input_geo_attr = ompx.cvar.MPxGeometryFilter_inputGeom
    handle_input = mdatablock.outputArrayValue(input_attr)
    handle_input.jumpToElement(geo_index)
    return handle_input.outputValue().child(input_geo_attr).asMesh()


def build_vertex_neighbours_dict(mesh, step=1):
    mesh_iterator = om.MItMeshVertex(mesh)
    connections = {}
    while not mesh_iterator.isDone():
        index = mesh_iterator.index()
        int_array = om.MIntArray()
        mesh_iterator.getConnectedVertices(int_array)
        connections[index] = int_array
        mesh_iterator.next()

    if step == 1:
        return connections

    neighbours_dict = {}
    mesh_iterator.reset()
    while not mesh_iterator.isDone():
        vertex_id = mesh_iterator.index()
        vertex_ids = {vertex_id}
        expand = []
        for _ in range(step):
            if not expand:
                expand = connections[vertex_id]
                vertex_ids.update(expand)
                continue
            expand = [
                v for vtx_id in expand
                for v in connections[vtx_id]
                if v not in vertex_ids]
            vertex_ids.update(expand)
        neighbours_dict[vertex_id] = vertex_ids
        mesh_iterator.next()
    return neighbours_dict


def is_meshes_match(*fn_meshes):
    fn_mesh_ref = None
    for fn_mesh in fn_meshes:
        if fn_mesh.isNull():
            return False
        if fn_mesh_ref is None:
            fn_mesh_ref = fn_mesh
            continue
        conditions = (
            fn_mesh_ref.numVertices == fn_mesh.numVertices and
            fn_mesh_ref.numEdges == fn_mesh.numEdges and
            fn_mesh_ref.numPolygons == fn_mesh.numPolygons and
            fn_mesh_ref.numFaceVertices == fn_mesh.numFaceVertices)
        if not all(conditions):
            return False
    return True
