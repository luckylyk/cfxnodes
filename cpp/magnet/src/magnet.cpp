#include <maya/MPxDeformerNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MItGeometry.h>
#include <maya/MDataBlock.h>
#include <maya/MFnMesh.h>
#include <maya/MTypes.h>
#include <maya/MPoint.h>
#include <maya/MFloatVectorArray.h>
#include <math.h>


double computeFactor(double const min, double const max, double dist) {
    if (dist < min) {
        return 0.0;}
    if (dist > max) {
        return 1.0;}
    double normalizeMax(max - min);
    double factor((dist - min) / normalizeMax);
    return factor;
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
        static MObject minInfluenceDistance;
        static MObject maxInfluenceDistance;
};


MTypeId Magnet::id(0x05254e);
MObject Magnet::magnetMesh;
MObject Magnet::minInfluenceDistance;
MObject Magnet::maxInfluenceDistance;
Magnet::Magnet() {}
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

    attributeAffects(magnetMesh, outputGeom);
    attributeAffects(minInfluenceDistance, outputGeom);
    attributeAffects(maxInfluenceDistance, outputGeom);
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
    MObject magMesh(dataBlock.inputValue(magnetMesh).asMesh());
    MFnMesh fnMagMesh(magMesh);
    MPoint closestPoint;
    double distance;
    double w;

    //inputmesh
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());
    MFnMesh fnMesh(inputMesh);

    if (magMesh.isNull() == true) {
        return MS::kSuccess;}

    for ( ; !vertIter.isDone(); vertIter.next()) {
        UINT index(vertIter.index());
        w = weightValue(dataBlock, multiIndex, index);
        MPoint position(vertIter.position());
        fnMagMesh.getClosestPoint(position, closestPoint, MSpace::kWorld);
        distance = position.distanceTo(closestPoint);
        double factor(computeFactor(min, max, distance));
        MPoint result;
        result.x = (position.x * factor) + (closestPoint.x * (1 - factor));
        result.y = (position.y * factor) + (closestPoint.y * (1 - factor));
        result.z = (position.z * factor) + (closestPoint.z * (1 - factor));
        result.x = (result.x * env) + (position.x * (1 - env));
        result.y = (result.y * env) + (position.y * (1 - env));
        result.z = (result.z * env) + (position.z * (1 - env));
        result.x = (result.x * w) + (position.x * (1 - w));
        result.y = (result.y * w) + (position.y * (1 - w));
        result.z = (result.z * w) + (position.z * (1 - w));

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

