
#include <cmath>
#include <vector>

#include <maya/MPxDeformerNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
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

#include "deformers.h"
#include "functions.h"


using namespace std;


MString Magnet::name("magnet");
MTypeId Magnet::id(0x05254e);
MObject Magnet::magnetMesh;
MObject Magnet::interpolation;
MObject Magnet::minInfluenceRadius;
MObject Magnet::maxInfluenceRadius;
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

    minInfluenceRadius = fnNumAttr.create(
        "minInfluenceRadius",
        "min",
        MFnNumericData::kFloat,
        1.0,
        &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setKeyable(true);
    addAttribute(minInfluenceRadius);

    maxInfluenceRadius = fnNumAttr.create(
        "maxInfluenceRadius",
        "max",
        MFnNumericData::kFloat,
        3.0,
        &status);
    fnNumAttr.setMin(0.01);
    fnNumAttr.setKeyable(true);
    addAttribute(maxInfluenceRadius);

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
    attributeAffects(minInfluenceRadius, outputGeom);
    attributeAffects(maxInfluenceRadius, outputGeom);
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
    float min(dataBlock.inputValue(minInfluenceRadius).asFloat());
    float max(dataBlock.inputValue(maxInfluenceRadius).asFloat());
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

        fnMagMesh.getClosestPointAndNormal(blendedClosestPoint, closestPoint, closestNormal, MSpace::kWorld);
        distance = position.distanceTo(closestPoint);
        angle = (normal.angle(closestNormal) / M_PI);
        angle = normalizeValue(vMinNormalInfluence, vMaxNormalInfluence, angle);
        angle += ((1 - angle) * (1 - normalInfluence));
        double factor(computeDistanceFactor(min, max, distance));
        MPoint result;
        double weight(env * w * (1 - factor) * angle);
        result.x = (closestPoint.x * weight) + (position.x * (1 - weight));
        result.y = (closestPoint.y * weight) + (position.y * (1 - weight));
        result.z = (closestPoint.z * weight) + (position.z * (1 - weight));
        result += closestNormal * (vOffset * weight);
        vertIter.setPosition(result);}
    return status;
}