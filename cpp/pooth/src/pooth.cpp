// The pooth node is a combinason of a push deformer and a post smooth deformation
// Push + Smooth = Pooth ;)


#include <vector>
#include <string>
#include <algorithm>

#include <maya/MFnPlugin.h>
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
        neightbors[index] = connections;
        }
    return neightbors;
}


class Pooth : public MPxDeformerNode {
    public:
        Pooth();
        virtual ~Pooth();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MTypeId id;
        static MObject aPush;
        static MObject aSmoothEnvelope;
        static MObject aSmoothStep;
        static MObject aStepIteration;

        vector<vector<int>> neightbourOfVertices;
        int backedStepValue;
};


MTypeId Pooth::id(0x05435d);
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


MStatus initializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object, "lbMD", "0.0", "Any");
    status = fnPlugin.registerNode(
        "pooth",
        Pooth::id,
        Pooth::creator,
        Pooth::initialize,
        MPxDeformerNode::kDeformerNode);
    // macro which check if the status is well MS::kSucces
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("pooth loaded");
    return status;
}


MStatus uninitializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object);
    status = fnPlugin.deregisterNode(Pooth::id);
    // macro which check if the status is well MS::kSucces
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("pooth unloaded");
    return status;
}

