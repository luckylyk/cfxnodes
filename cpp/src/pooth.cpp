// The pooth node is a combinason of a push deformer and a post smooth deformation
// Push + Smooth = Pooth ;)

#include <vector>

#include <maya/MpxDeformerNode.h>
#include <maya/MTypeId.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshVertex.h>
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDataHandle.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>

#include "deformers.h"
#include "functions.h"


using namespace std;


MTypeId Pooth::id(0x05435d);
MString Pooth::name("pooth");
MObject Pooth::aPush;
MObject Pooth::aSmoothEnvelope;
MObject Pooth::aSmoothStep;
Pooth::Pooth():backedStepValue(0){}
Pooth::~Pooth(){}
void* Pooth::creator(){return new Pooth();}


MStatus Pooth::deform(
        MDataBlock& dataBlock,
        MItGeometry& vertIt,
        const MMatrix& matrix,
        UINT multiIndex){

    MStatus status;
    // get inputmesh object
    MArrayDataHandle hInputMesh = dataBlock.outputArrayValue(input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = hInputMesh.jumpToElement(multiIndex);
    MDataHandle hInputElement(hInputMesh.outputValue(&status));
    MObject inputMesh(hInputElement.child(inputGeom).asMesh());
    MFnMesh fnMesh(inputMesh);
    MPointArray meshPoints(vertIt.count());
    MPointArray deformedPoints(vertIt.count());
    fnMesh.getPoints(meshPoints, MSpace::kObject);
    MFloatVectorArray normals;
    fnMesh.getVertexNormals(true, normals, MSpace::kObject);

    unsigned int index;
    float env(dataBlock.inputValue(envelope).asFloat());
    int step(dataBlock.inputValue(aSmoothStep).asInt());
    float push(dataBlock.inputValue(aPush).asFloat());
    float smooth(dataBlock.inputValue(aSmoothEnvelope).asFloat());
    float smoothMult(env * smooth);
    float w; // weight per vertex

    if (neightbourOfVertices.empty() || (step != backedStepValue)){
        backedStepValue = step;
        neightbourOfVertices = computeNeighborhoodOfVertices(inputMesh, step);
        MGlobal::displayInfo("Neighborhood computed");
    }

    //compute push
    for (; !vertIt.isDone(); vertIt.next()) {
        index = vertIt.index();
        w = weightValue(dataBlock, multiIndex, index);
        meshPoints[index] += normals[index] * (push * env * w);}

    //compute smooth
    vertIt.reset();
    for (; !vertIt.isDone(); vertIt.next()) {
        index = vertIt.index();
        w = weightValue(dataBlock, multiIndex, index);
        MPoint finalPoint = meshPoints[index];
        MPoint averagePoint;
        for (int i(0); i < neightbourOfVertices[index].size(); ++i) {
            averagePoint.x += (meshPoints[neightbourOfVertices[index][i]].x / neightbourOfVertices[index].size());
            averagePoint.y += (meshPoints[neightbourOfVertices[index][i]].y / neightbourOfVertices[index].size());
            averagePoint.z += (meshPoints[neightbourOfVertices[index][i]].z / neightbourOfVertices[index].size());}
        float mult(smoothMult * w);
        finalPoint.x = (finalPoint.x * (1 - mult)) + (averagePoint.x * mult);
        finalPoint.y = (finalPoint.y * (1 - mult)) + (averagePoint.y * mult);
        finalPoint.z = (finalPoint.z * (1 - mult)) + (averagePoint.z * mult);
        deformedPoints[index] = finalPoint;}
    vertIt.setAllPositions(deformedPoints);
    return MS::kSuccess;
}


MStatus Pooth::initialize(){
    MFnNumericAttribute fnNumAttr;
    MStatus status;

    aPush = fnNumAttr.create("push", "ps", MFnNumericData::kFloat, 0.0, &status);
    fnNumAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aPush);
    attributeAffects(aPush, outputGeom);

    aSmoothEnvelope = fnNumAttr.create("smoothEnvelope", "sm", MFnNumericData::kFloat, 0.0, &status);
    fnNumAttr.setMin(0);
    fnNumAttr.setMax(1);
    fnNumAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aSmoothEnvelope);
    attributeAffects(aSmoothEnvelope, outputGeom);

    aSmoothStep = fnNumAttr.create("smoothStep", "ss", MFnNumericData::kInt, 1, &status);
    fnNumAttr.setMin(1);
    fnNumAttr.setMax(10);
    fnNumAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(aSmoothStep);
    attributeAffects(aSmoothStep, outputGeom);

    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer pooth weights;");
    return status;
}

