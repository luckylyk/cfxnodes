
#include <cmath>
#include <vector>

#include <maya/MPxDeformerNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MItGeometry.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MItMeshVertex.h>
#include <maya/MIntArray.h>


using namespace std;


vector<vector<int>> computeNeighborhoodOfVertices(MObject &mesh, int stepRadius) {
    stepRadius -= 1;

    MItMeshVertex vertIt(mesh); // iterator to parse the mesh and store the neightbors
    MItMeshVertex vertIt2(mesh); // iterator used to jump indexes in the recusive neightbors gathering
    vector<vector<int>> neightbors(vertIt.count()); // result pre definition
    vector<int> connections; // sub result definition

    // define buffer arrays
    MIntArray step;
    MIntArray vertices;
    MIntArray buffer;

    unsigned int index;

    for (; !vertIt.isDone(); vertIt.next()) {
        index = vertIt.index();
        vertIt.getConnectedVertices(step);
        connections.clear();

        if (stepRadius == 0) {
            connections.resize(step.length());
            for (unsigned int i(0); i < step.length(); ++i){
                connections[i] = step[i];}
            neightbors[index] = connections;
            continue;}

        for (unsigned int i(0); i < step.length(); ++i){
            connections.push_back(step[i]);}

        for (int i(0); i < stepRadius; ++i) {

            buffer.clear();
            for (unsigned int j(0); j < step.length(); ++j) {
                int old_useless_index;
                vertIt2.setIndex(step[j], old_useless_index);
                vertIt2.getConnectedVertices(vertices);
                for (unsigned int k(0); k < vertices.length(); ++k){
                    __int64 occurency;
                    occurency = count(connections.begin(), connections.end(), vertices[k]);
                    if (occurency == 0){
                        connections.push_back(vertices[k]);
                        buffer.append(vertices[k]);}}
                }
            step = buffer;}
        neightbors[index] = connections;}
    return neightbors;
}


double computeFactor(double const min, double const max, double dist) {
    if (dist < min) {
        return 0.0;}
    if (dist > max) {
        return 1.0;}
    double normalizeMax(max - min);
    double factor((dist - min) / normalizeMax);
    return factor;
}


double clamp(double const min, double const max, double const value) {
    if (value < min) {return 0.0;}
    if (value > max) {return 1.0;}
    double range(1 - ((1-max) + min));
    double normValue(value - min);
    return normValue / range;
}


class Magnet : public MPxDeformerNode {
    public:
        Magnet();
        virtual ~Magnet();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MTypeId id;
        static MObject magnetMesh;
        static MObject interpolation;
        static MObject minInfluenceDistance;
        static MObject maxInfluenceDistance;
        static MObject normalAngleInfluence;
        static MObject minNormalInfluence;
        static MObject maxNormalInfluence;
        static MObject offset;

        vector<vector<int>> neightbourOfVertices;
        int backedInterpolation;
};


MTypeId Magnet::id(0x05254e);
MObject Magnet::magnetMesh;
MObject Magnet::interpolation;
MObject Magnet::minInfluenceDistance;
MObject Magnet::maxInfluenceDistance;
MObject Magnet::normalAngleInfluence;
MObject Magnet::minNormalInfluence;
MObject Magnet::maxNormalInfluence;
MObject Magnet::offset;
Magnet::Magnet(): backedInterpolation(-1) {}
Magnet::~Magnet() {}
void* Magnet::creator(){return new Magnet();}
MStatus Magnet::initialize() {

    MFnTypedAttribute fnTypedAttr;
    MFnNumericAttribute fnNumAttr;
    MStatus status;

    magnetMesh = fnTypedAttr.create(
        "magnetMesh",
        "mm",
        MFnData::kMesh,
        &status);
    addAttribute(magnetMesh);

    interpolation = fnNumAttr.create(
        "interpolation",
        "i",
        MFnNumericData::kInt,
        1.0,
        &status);
    fnNumAttr.setMin(1);
    fnNumAttr.setMax(10);
    fnNumAttr.setKeyable(true);
    addAttribute(interpolation);

    minInfluenceDistance = fnNumAttr.create(
        "minInfluenceDistance",
        "min",
        MFnNumericData::kFloat,
        1.0,
        &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setKeyable(true);
    addAttribute(minInfluenceDistance);

    maxInfluenceDistance = fnNumAttr.create(
        "maxInfluenceDistance",
        "max",
        MFnNumericData::kFloat,
        3.0,
        &status);
    fnNumAttr.setMin(0.01);
    fnNumAttr.setKeyable(true);
    addAttribute(maxInfluenceDistance);

    normalAngleInfluence = fnNumAttr.create(
        "normalAngleInfluence",
        "nai",
        MFnNumericData::kFloat,
        .5,
        &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setMax(1);
    fnNumAttr.setKeyable(true);
    addAttribute(normalAngleInfluence);

    minNormalInfluence = fnNumAttr.create(
        "minNormalInfluence",
        "mini",
        MFnNumericData::kFloat,
        0.1,
        &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setMax(1);
    fnNumAttr.setKeyable(true);
    addAttribute(minNormalInfluence);

    maxNormalInfluence = fnNumAttr.create(
        "maxNormalInfluence",
        "maxi",
        MFnNumericData::kFloat,
        .9,
        &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setMax(1);
    fnNumAttr.setKeyable(true);
    addAttribute(maxNormalInfluence);

    offset = fnNumAttr.create(
        "offset",
        "o",
        MFnNumericData::kFloat,
        0.0,
        &status);
    fnNumAttr.setKeyable(true);
    addAttribute(offset);

    attributeAffects(magnetMesh, outputGeom);
    attributeAffects(interpolation, outputGeom);
    attributeAffects(minInfluenceDistance, outputGeom);
    attributeAffects(maxInfluenceDistance, outputGeom);
    attributeAffects(normalAngleInfluence, outputGeom);
    attributeAffects(minNormalInfluence, outputGeom);
    attributeAffects(maxNormalInfluence, outputGeom);
    attributeAffects(offset, outputGeom);
    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer magnet weights;");

    return MS::kSuccess;
}


MStatus Magnet::deform(
        MDataBlock& dataBlock,
        MItGeometry& vertIter,
        const MMatrix& matrix,
        UINT multiIndex) {

    MStatus status;

    float env(dataBlock.inputValue(envelope).asFloat());
    float min(dataBlock.inputValue(minInfluenceDistance).asFloat());
    float max(dataBlock.inputValue(maxInfluenceDistance).asFloat());
    int vInterpolation(dataBlock.inputValue(interpolation).asInt());
    float normalInfluence(dataBlock.inputValue(normalAngleInfluence).asFloat());
    float vMinNormalInfluence(dataBlock.inputValue(minNormalInfluence).asFloat());
    float vMaxNormalInfluence(dataBlock.inputValue(maxNormalInfluence).asFloat());
    float vOffset(dataBlock.inputValue(offset).asFloat());

    MObject magMesh(dataBlock.inputValue(magnetMesh).asMesh());
    MFnMesh fnMagMesh(magMesh);
    MPoint closestPoint;
    MVector closestNormal;
    double distance;
    double w;
    double angle;

    //inputmesh
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());
    MFnMesh fnMesh(inputMesh);
    MItMeshVertex iterInputVertices(inputMesh);
    if (magMesh.isNull() == true) {
        return MS::kSuccess;}

    MFloatVectorArray normals(vertIter.count());
    fnMesh.getNormals(normals);
    MFloatVectorArray closestNormals(vertIter.count());
    MPointArray closestPoints(vertIter.count());

    if (neightbourOfVertices.empty() || (vInterpolation != backedInterpolation)){
        backedInterpolation = vInterpolation;
        neightbourOfVertices = computeNeighborhoodOfVertices(inputMesh, vInterpolation);
        MGlobal::displayInfo("Neighborhood computed");}

    // store datas
    for ( ; !vertIter.isDone(); vertIter.next()) {
        UINT index(vertIter.index());
        MPoint position(vertIter.position());
        fnMagMesh.getClosestPoint(position, closestPoint, MSpace::kWorld);
        closestPoints[index] = closestPoint;}

    int dummy;
    // compute deformation
    vertIter.reset();
    for ( ; !vertIter.isDone(); vertIter.next()) {
        UINT index(vertIter.index());
        MVector closestNormal;
        MVector normal(normals[index]);
        MPoint blendedClosestPoint;
        for (int i(0); i < neightbourOfVertices[index].size(); ++i) {
            iterInputVertices.setIndex(neightbourOfVertices[index][i], dummy);
            blendedClosestPoint += closestPoints[neightbourOfVertices[index][i]];}
        blendedClosestPoint = blendedClosestPoint / neightbourOfVertices[index].size();
        MPoint position(vertIter.position());

        w = weightValue(dataBlock, multiIndex, index);
        fnMagMesh.getClosestPointAndNormal(blendedClosestPoint, closestPoint, closestNormal, MSpace::kWorld);
        distance = position.distanceTo(closestPoint);
        angle = (normal.angle(closestNormal) / M_PI);
        angle = clamp(vMinNormalInfluence, vMaxNormalInfluence, angle);
        angle += ((1 - angle) * (1 - normalInfluence));
        double factor(computeFactor(min, max, distance));
        MPoint result;
        double weight(env * w * (1 - factor) * angle);
        result.x = (closestPoint.x * weight) + (position.x * (1 - weight));
        result.y = (closestPoint.y * weight) + (position.y * (1 - weight));
        result.z = (closestPoint.z * weight) + (position.z * (1 - weight));
        result += closestNormal * (vOffset * weight);
        vertIter.setPosition(result);}
    return status;
}


MStatus initializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object, "lbMD", "0.0", "Any");
    status = fnPlugin.registerNode(
        "magnet",
        Magnet::id,
        Magnet::creator,
        Magnet::initialize,
        MPxDeformerNode::kDeformerNode);
    // macro which check if the status is well MS::kSucces
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("Magnet loaded");
    return status;
}


MStatus uninitializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object);
    status = fnPlugin.deregisterNode(Magnet::id);
    // macro which check if the status is well MS::kSucces
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("Magnet unloaded");
    return status;
}

