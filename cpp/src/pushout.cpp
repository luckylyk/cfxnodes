
#include <cmath>

#include "deformers.h"

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


MString PushOut::name("pushOut");
MTypeId PushOut::id(0x05426e);
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
    float radiusOffset(dataBlock.inputValue(aRadiusOffset).asFloat());
    float distanceMax(dataBlock.inputValue(aDistanceMax).asFloat());

    // get input
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());

    MFnMesh fnMeshInput(inputMesh), fnMeshPusher(oPusher);
    MPoint position, closestPoint, distanceReference, deformedPoint;
    MVector closestNormal, median, behindCheck;
    double distance, angle, mult, w;
    MFloatVectorArray normals;
    fnMeshInput.getNormals(normals);
    UINT index;
    for (; !vertIter.isDone(); vertIter.next()) {
        index = vertIter.index();
        w = weightValue(dataBlock, multiIndex, index);
        if (w == 0) {
            continue;}

        position = vertIter.position(MSpace::kWorld);
        MVector vectMult(normals[index] * radiusOffset);
        distanceReference = vertIter.position() - vectMult;
        fnMeshPusher.getClosestPointAndNormal(position, closestPoint, closestNormal, MSpace::kWorld);

        angle = closestNormal.angle(normals[index]);
        distance = position.distanceTo(closestPoint);
        if (angle > (M_PI / 4) || distance > distanceMax) {
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
    }
    return MS::kSuccess;
}
