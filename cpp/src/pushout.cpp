
#include <cmath>

#include <maya/MPxDeformerNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MDataHandle.h>
#include <maya/MFnMesh.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MItGeometry.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MVector.h>
#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MItMeshVertex.h>

#include "deformers.h"
#include "functions.h"


MString PushOut::name("pushOut");
MTypeId PushOut::id(0x05426e);
MObject PushOut::interpolation;
MObject PushOut::aPusher;
MObject PushOut::aPush;
MObject PushOut::aRadiusOffset;
MObject PushOut::aDistanceMax;
PushOut::PushOut() {}
PushOut::~PushOut() {}
void* PushOut::creator() {return new PushOut();}
MStatus PushOut::initialize() {

    MStatus status;
    MFnNumericAttribute fnNumAttr;
    MFnTypedAttribute fnTypedAttr;

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
    attributeAffects(interpolation, outputGeom);

    aPusher = fnTypedAttr.create("pusher", "p", MFnData::kMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aPusher);
    attributeAffects(aPusher, outputGeom);

    aPush = fnNumAttr.create("push", "ps", MFnNumericData::kFloat, 0.1, &status);
    fnNumAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aPush);
    attributeAffects(aPush, outputGeom);

    aRadiusOffset = fnNumAttr.create("radiusOffset", "ro", MFnNumericData::kFloat, -1.5, &status);
    fnNumAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aRadiusOffset);
    attributeAffects(aRadiusOffset, outputGeom);

    aDistanceMax = fnNumAttr.create("distanceMax", "dm", MFnNumericData::kFloat, 2, &status);
    fnNumAttr.setKeyable(true);
    fnNumAttr.setMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aDistanceMax);
    attributeAffects(aDistanceMax, outputGeom);

    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer pushOut weights;");
    return status;
}


MStatus PushOut::deform(
        MDataBlock& dataBlock,
        MItGeometry& vertIter,
        const MMatrix& matrix,
        UINT multiIndex) {

    MStatus status;

    float env(dataBlock.inputValue(envelope).asFloat());
    MObject oPusher(dataBlock.inputValue(aPusher).asMesh());
    float push(dataBlock.inputValue(aPush).asFloat());
    int vInterpolation(dataBlock.inputValue(interpolation).asInt());
    float radiusOffset(dataBlock.inputValue(aRadiusOffset).asFloat());
    float distanceMax(dataBlock.inputValue(aDistanceMax).asFloat());

    // get input
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());

    MFnMesh fnMeshInput(inputMesh), fnMeshPusher(oPusher);
    MItMeshVertex iterInputVertices(inputMesh);
    MPoint position, closestPoint, distanceReference, deformedPoint, blendedClosestPoint;
    MVector closestNormal, median, behindCheck, vectMult;
    double distance, angle, mult, w;
    UINT index;
    int dummy;

    if (oPusher.isNull()) {
        return MS::kSuccess;}

    if (neightbourOfVertices.empty() || (vInterpolation != backedInterpolation)){
        backedInterpolation = vInterpolation;
        neightbourOfVertices = computeNeighborhoodOfVertices(inputMesh, vInterpolation);
        MGlobal::displayInfo("Neighborhood computed");}

    MFloatVectorArray normals(vertIter.count());
    fnMeshInput.getNormals(normals);
    MFloatVectorArray closestNormals(vertIter.count());
    MPointArray closestPoints(vertIter.count());

    // store datas
    for ( ; !vertIter.isDone(); vertIter.next()) {
        index = vertIter.index();
        MPoint position(vertIter.position(MSpace::kWorld));
        fnMeshPusher.getClosestPoint(position, closestPoint, MSpace::kWorld);
        closestPoints[index] = closestPoint;}

    vertIter.reset();
    for (; !vertIter.isDone(); vertIter.next()) {
        index = vertIter.index();
        w = weightValue(dataBlock, multiIndex, index);
        if (w == 0) {
            continue;}

        for (int i(0); i < neightbourOfVertices[index].size(); ++i) {
            iterInputVertices.setIndex(neightbourOfVertices[index][i], dummy);
            blendedClosestPoint += closestPoints[neightbourOfVertices[index][i]];}
        blendedClosestPoint = blendedClosestPoint / neightbourOfVertices[index].size();

        position = vertIter.position(MSpace::kWorld);
        vectMult = normals[index] * radiusOffset;
        distanceReference = vertIter.position(MSpace::kWorld) - vectMult;
        fnMeshPusher.getClosestPointAndNormal(blendedClosestPoint, closestPoint, closestNormal, MSpace::kWorld);

        angle = closestNormal.angle(normals[index]);
        distance = position.distanceTo(closestPoint);
        if (angle > (M_PI / 4) || distance > distanceMax) { //(angle > (M_PI / 4) || distance > distanceMax) {
            continue;}

        median = (closestNormal + normals[index]) / 2;
        deformedPoint = closestPoint + (median * push);

        //this is computing is the computed point is behind the original one
        // if it is the case, it is not necessary to process the pushout.
        behindCheck = position - deformedPoint;
        angle = behindCheck.angle(normals[index]);
        if (angle < (M_PI / 2)) {
            continue;}

        mult = env * w;
        deformedPoint.x = (position.x * (1 - mult)) + (deformedPoint.x * mult);
        deformedPoint.y = (position.y * (1 - mult)) + (deformedPoint.y * mult);
        deformedPoint.z = (position.z * (1 - mult)) + (deformedPoint.z * mult);
        vertIter.setPosition(deformedPoint);
    };
    return MS::kSuccess;
}
