
#include <vector>
#include <utility>
#include <string>
#include "meshmatch.h"

#include <maya/MGlobal.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTypeId.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshVertex.h>
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnNumericData.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDataHandle.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>

using namespace std;


pair<unsigned int, double> getClosestVertexIndexAndDistance(
        MPoint const& refPoint,
        MObject& mesh) {

    double distanceBuffer;
    MPoint closestPoint;
    int closestVertexIndex(0);
    MFnMesh fnMesh(mesh);
    MItMeshVertex vertIt(mesh);
    double closestDistance(numeric_limits<double>::max());

    fnMesh.getClosestPoint(refPoint, closestPoint, MSpace::Space::kWorld);
    unsigned int i;
    for (; !vertIt.isDone(); vertIt.next()) {
        i = vertIt.index();
        MPoint currentPoint(vertIt.position(MSpace::Space::kWorld));
        distanceBuffer = currentPoint.distanceTo(closestPoint);
        if (distanceBuffer < closestDistance) {
            closestDistance = distanceBuffer;
            closestVertexIndex = i;}}
    pair<unsigned int, double> result(closestVertexIndex, closestDistance);
    return result;
}


MTypeId MeshMatch::id(0x05425d);
MObject MeshMatch::aMeshes;
MObject MeshMatch::aMatchesGeoIndex;
MObject MeshMatch::aMatchesVertexId;
MObject MeshMatch::hasToRebuiltMatches;
MeshMatch::MeshMatch(){}
MeshMatch::~MeshMatch(){}
void* MeshMatch::creator(){return new MeshMatch();}


void MeshMatch::buildMatches(MDataBlock &dataBlock, MObject &inputMesh){
    MItMeshVertex itInputVert(inputMesh);
    MObject myNode(thisMObject());
    MPlug pMeshes(myNode, aMeshes);
    MPlug pMesh;
    MPoint refPoint;
    unsigned int index;
    MIntArray geoIndices(itInputVert.count());
    MIntArray vertexIndices(itInputVert.count());
    vector<double> distancesStorage(itInputVert.count(), numeric_limits<double>::max());

    for (int geoI(pMeshes.numElements()); geoI >= 0; --geoI) {;
        pMesh = pMeshes.elementByLogicalIndex(geoI);
        if (pMesh.isConnected() == false) {
            continue;}
        MDataHandle hMesh(dataBlock.inputValue(pMesh));
        MObject oMeshMatcher(hMesh.asMesh());
        itInputVert.reset();
        for (; !itInputVert.isDone(); itInputVert.next()){
            index = itInputVert.index();
            refPoint = itInputVert.position(MSpace::Space::kWorld);
            pair <unsigned int, double> closestVertexIndexAndDistance(
                getClosestVertexIndexAndDistance(refPoint, oMeshMatcher));
            if (closestVertexIndexAndDistance.second < distancesStorage[index]) {
                distancesStorage[index] = closestVertexIndexAndDistance.second;
                vertexIndices[index] = closestVertexIndexAndDistance.first;
                geoIndices[index] = geoI;}}}

    MDataHandle hGeoIndices(dataBlock.outputValue(aMatchesGeoIndex));
    MFnIntArrayData arrayGeoIndices(hGeoIndices.data());
    arrayGeoIndices.set(geoIndices);
    hGeoIndices.setClean();

    MDataHandle hVertexIndices(dataBlock.outputValue(aMatchesVertexId));
    MFnIntArrayData arrayVertexIndices(hVertexIndices.data());
    arrayVertexIndices.set(vertexIndices);
    hVertexIndices.setClean();
}


MStatus MeshMatch::deform(
        MDataBlock& dataBlock,
        MItGeometry& vertIt,
        const MMatrix& matrix,
        UINT multiIndex){

    float env(dataBlock.inputValue(envelope).asFloat());
    MDataHandle hRebuilt(dataBlock.inputValue(hasToRebuiltMatches));
    bool rebuilt(hRebuilt.asBool());

    MStatus status;
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());
    if (rebuilt == true) {
        buildMatches(dataBlock, inputMesh);
        hRebuilt.setBool(false);
        MGlobal::displayInfo("matches built");}

    MPointArray deformedPoints(vertIt.count());
    MDataHandle hGeoIndices(dataBlock.outputValue(aMatchesGeoIndex));
    MFnIntArrayData arrayGeoIndices(hGeoIndices.data());
    MIntArray geoIndices;
    arrayGeoIndices.copyTo(geoIndices);

    MDataHandle hVertexIndices(dataBlock.outputValue(aMatchesVertexId));
    MFnIntArrayData arrayVertexIndices(hVertexIndices.data());
    MIntArray vertexIndices;
    arrayVertexIndices.copyTo(vertexIndices);

    MObject myNode(thisMObject());
    MPlug pMeshes(myNode, aMeshes);
    MPlug pMesh;
    int dummy;

    for (unsigned int geoI(0); geoI < pMeshes.numElements(); ++geoI) {
        pMesh = pMeshes.elementByLogicalIndex(geoI);
        if (pMesh.isConnected() == false) {
            continue;}
        MDataHandle hMesh(dataBlock.inputValue(pMesh));
        MObject oMeshMatcher(hMesh.asMesh());
        MItMeshVertex itVtxMeshMatcher(oMeshMatcher);
        vertIt.reset();
        for (; !vertIt.isDone(); vertIt.next()){
            unsigned int index = vertIt.index();
            if (geoI == geoIndices[index]) {
                itVtxMeshMatcher.setIndex(vertexIndices[index], dummy);
                MPoint targetPoint(itVtxMeshMatcher.position(MSpace::Space::kWorld));
                MPoint originalPoint(vertIt.position());
                MPoint finalPoint;
                finalPoint.x = (targetPoint.x * env) + (originalPoint.x * (1 - env));
                finalPoint.y = (targetPoint.y * env) + (originalPoint.y * (1 - env));
                finalPoint.z = (targetPoint.z * env) + (originalPoint.z * (1 - env));
                deformedPoints[index] = finalPoint;}}}

    vertIt.setAllPositions(deformedPoints);
    return MS::kSuccess;
}


MStatus MeshMatch::initialize(){
    MFnNumericAttribute fnNumAttr;
    MFnTypedAttribute fnTypedAttr;
    MFnIntArrayData fnIntArray;
    MIntArray defaultIntArray;
    fnIntArray.create(defaultIntArray);
    MStatus status;

    aMeshes = fnTypedAttr.create("meshes", "ms", MFnData::kMesh, &status);
    fnTypedAttr.setArray(true);
    fnTypedAttr.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aMeshes);
    attributeAffects(aMeshes, outputGeom);

    aMatchesGeoIndex = fnTypedAttr.create(
        "matchesGeoIndex", "mgi",
        MFnData::kIntArray,
        fnIntArray.object(),
        &status);
    addAttribute(aMatchesGeoIndex);

    aMatchesVertexId = fnTypedAttr.create(
        "matchesVertexId", "mvi",
        MFnData::kIntArray,
        fnIntArray.object(),
        &status);
    addAttribute(aMatchesVertexId);

    hasToRebuiltMatches = fnNumAttr.create(
        "hasToRebuiltMatches",
        "htrm",
        MFnNumericData::kBoolean,
        true,
        &status);
    fnNumAttr.setInternal(true);
    fnNumAttr.setHidden(true);
    addAttribute(hasToRebuiltMatches);
    attributeAffects(hasToRebuiltMatches, outputGeom);


    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer meshMatch weights;");

    MGlobal::displayInfo("inited");
    return status;
}