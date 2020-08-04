
#include <vector>
#include <utility>
#include <string>

#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MPxCommand.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MpxDeformerNode.h>
#include <maya/MTypeId.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshVertex.h>
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>
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


class MeshMatch : public MPxDeformerNode {
    public:
        MeshMatch();
        virtual ~MeshMatch();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        void buildMatches(MDataBlock& dataBlock, MObject& inputMesh);
        static MTypeId id;
        static MObject aMeshes;
        static MObject aMatchesGeoIndex;
        static MObject aMatchesVertexId;
        static MObject hasToRebuiltMatches;
};


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


class MeshMatchCommand: public MPxCommand
    {
    public:
        MeshMatchCommand();
        ~MeshMatchCommand() override;
        MStatus doIt(const MArgList& args) override;
        static MSyntax createSyntax();
        static void* creator();
};


MeshMatchCommand::MeshMatchCommand() {}
MeshMatchCommand::~MeshMatchCommand() {}
void* MeshMatchCommand::creator() {return new MeshMatchCommand();}


void checkArgsSanity(
        const MArgList & argList,
        MArgParser & argParser,
        MString & meshMatchNodeName,
        MStatus & status) {

    if (argParser.numberOfFlagsUsed() > 1) {
        status = MS::kInvalidParameter;
        MString msg("meshMatch doesn't support multi-flag usage.");
        MGlobal::displayError(msg);
        return;}

    if (argParser.numberOfFlagsUsed() == 0) {
        MStringArray objects;
        argParser.getObjects(objects);
        if (objects.length() < 2) {
            status = MS::kInvalidParameter;
            MString msg(
                "No enough objects providen.\n At least 2 geometries "
                "must be selected or passed as argument when the command is "
                "used without any flag or");
            MGlobal::displayError(msg);
            return;}}

    if (argParser.isFlagSet("updateBinding") == true) {
        meshMatchNodeName = argParser.flagArgumentString("updateBinding", 0);
        if (argList.length() != 0){
            status = MS::kInvalidParameter;
            MString msg(
                "meshMatch command doesn't support argument with flag "
                "-updateBinding");
            MGlobal::displayError(msg);
            return;}}

    if (argParser.isFlagSet("addGeometryDriver") == true) {
        meshMatchNodeName = argParser.flagArgumentString("addGeometryDriver", 0);
        if (argList.length() == 0){
            status = MS::kInvalidParameter;
            MString msg(
                "meshMatch need at least one geometry argument with "
                "flag addGeometryDriver");
            MGlobal::displayError(msg);
            return;}}

    if (argParser.isFlagSet("removeGeometryDriver") == true) {
        meshMatchNodeName = argParser.flagArgumentString("removeGeometryDriver", 0);
        if (argList.length() == 0){
            status = MS::kInvalidParameter;
            MString msg(
                "meshMatch need at least one geometry argument with flag "
                "removeGeometryDriver");
            MGlobal::displayError(msg);
            return;}}
    return;
}


MStatus toDependecyNode(
        const MString & nodeName,
        MObject & mobject,
        MStatus status){

    MSelectionList nodelist;
    status = nodelist.add(nodeName);
    if (status != MS::kSuccess) {
        MGlobal::displayError(nodeName + " doesn't exists");}
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MDagPath dagPath;
    nodelist.getDagPath(0, dagPath);
    dagPath.extendToShape();
    MObject oDependNode(dagPath.node());
    mobject = oDependNode;
    return status;
}


MStatus isNodeType(
        const MString & nodeName,
        const MString & nodeType,
        MStatus & status){

    MObject oDependNode;
    toDependecyNode(nodeName, oDependNode, status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MFnDependencyNode fnDagNode(oDependNode);

    if (fnDagNode.typeName() != nodeType) {
        MString msg(
            nodeName + " must be a " + nodeType +
            " node but is " + fnDagNode.typeName());
        MGlobal::displayInfo(msg);
        status = MS::kInvalidParameter;}
    return status;
}


void getSelectedMeshes(MStringArray & geometries, MStatus & status) {
    MSelectionList sel;
    MGlobal::getActiveSelectionList(sel);
    MStringArray selections;
    sel.getSelectionStrings(selections);
    bool isMesh;
    for (unsigned int i(0); i < selections.length(); ++i) {
        isMesh = isNodeType(selections[i], "mesh", status);
        if (isMesh == true) {
            geometries.append(selections[i]);}}
    return;
}


unsigned int findNextFreeLogicalIndex(
        MPlug const & plugArray,
        unsigned int index) {
    MPlug child;
    while (true) {
        child = plugArray.elementByLogicalIndex(index);
        if (child.isConnected() == false) {
            return index;}
        index += 1;}
    return index;
}


void addGeometriesToMeshMatch(
        const MStringArray & geometries,
        const MString & meshMatchName,
        MStatus & status) {
    MDGModifier modifier;
    MPlug dstPlug;
    MPlug srcPlug;
    MPlug pMeshes;
    MSelectionList sel;
    sel.add(meshMatchName + ".meshes");
    sel.getPlug(0, pMeshes);
    unsigned int plugChildIndex(0);
    for (unsigned int i(0); i < geometries.length(); ++i) {
        MString geometry(geometries[i]);
        MObject oGeometry;
        toDependecyNode(geometry, oGeometry, status);
        plugChildIndex = findNextFreeLogicalIndex(pMeshes, plugChildIndex);
        MFnDependencyNode fnDepNodeGeometry(oGeometry);
        srcPlug = fnDepNodeGeometry.findPlug("outMesh", status);
        dstPlug = pMeshes.elementByLogicalIndex(plugChildIndex);
        status = modifier.connect(srcPlug, dstPlug);
        plugChildIndex += 1;}
    status = modifier.doIt();
    return;
}


void removeGeometriesToMeshMatch(
        const MStringArray & geometries,
        const MString meshMatchName,
        MStatus & status) {

    MDGModifier modifier;
    MPlug pMeshes;
    MPlug dstPlug;
    MPlug srcPlug;
    MSelectionList sel;

    sel.add(meshMatchName + ".meshes");
    sel.getPlug(0, pMeshes);

    for (unsigned int i(0); i < geometries.length(); ++i) {
        MString geometry(geometries[i]);
        MObject oGeometry;
        toDependecyNode(geometry, oGeometry, status);
        MFnDependencyNode fnDepNodeGeometry(oGeometry);
        srcPlug = fnDepNodeGeometry.findPlug("outMesh");
        for (unsigned int j(0); j < pMeshes.numElements(); ++j) {
            dstPlug = pMeshes.elementByLogicalIndex(j);
            MPlugArray connections;
            connections.append(dstPlug);
            srcPlug.connectedTo(connections, false, true);
            for (unsigned int k(0); k < connections.length(); ++k) {
                if (dstPlug.name() == connections[k].name()) {
                    modifier.disconnect(srcPlug, dstPlug);
                    modifier.doIt();}}}}
}


MString createMeshMatchs(const MString & target, MStatus & status) {
    MString command;
    command += "deformer -type meshMatch " + target;
    MStringArray result;
    MGlobal::executeCommand(command, result);
    return result[0];
}


MStatus checksNodesTypesSanity(
        const MStringArray & geometries,
        MStatus & status){

    for (unsigned int i(0); i < geometries.length(); ++i){
        MString geometry(geometries[i]);
        isNodeType(geometry, "mesh", status);
        CHECK_MSTATUS_AND_RETURN_IT(status);}
    return status;
}


MStatus MeshMatchCommand::doIt(const MArgList & args) {
    MStatus status;
    MArgParser argParser(syntax(), args, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString meshMatchNodeName;
    checkArgsSanity(args, argParser, meshMatchNodeName, status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray geometries;
    argParser.getObjects(geometries);
    if (geometries.length() == 0) {
        getSelectedMeshes(geometries, status);
        CHECK_MSTATUS_AND_RETURN_IT(status);}

    checksNodesTypesSanity(geometries, status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString target(geometries[geometries.length() - 1]);

    if (argParser.isFlagSet("addGeometryDriver")){
        addGeometriesToMeshMatch(geometries, meshMatchNodeName, status);
        CHECK_MSTATUS_AND_RETURN_IT(status);}

    if (argParser.isFlagSet("removeGeometryDriver")){
        removeGeometriesToMeshMatch(geometries, meshMatchNodeName, status);
        CHECK_MSTATUS_AND_RETURN_IT(status);}

    if (argParser.numberOfFlagsUsed() == 0) {
        geometries.remove(geometries.length() - 1);
        MString deformer(createMeshMatchs(target, status));
        CHECK_MSTATUS_AND_RETURN_IT(status);
        addGeometriesToMeshMatch(geometries, deformer, status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        setResult(deformer);}

    // set node in update mode
    MSelectionList sel;
    MPlug plug;
    sel.add(meshMatchNodeName + ".hasToRebuiltMatches");
    sel.getPlug(0, plug);
    plug.setValue(true);

    return MS::kSuccess;
}


MSyntax MeshMatchCommand::createSyntax(){
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.setObjectType(MSyntax::kStringObjects, 0, 1024);

    syntax.addFlag("-ub", "-updateBinding", MSyntax::kString);
    syntax.addFlag("-add", "-addGeometryDriver", MSyntax::kString);
    syntax.addFlag("-rmv", "-removeGeometryDriver", MSyntax::kString);
    return syntax;
}


MStatus initializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object, "meshmatch", "0.0", "Any");
    status = fnPlugin.registerNode(
        "meshMatch",
        MeshMatch::id,
        MeshMatch::creator,
        MeshMatch::initialize,
        MPxDeformerNode::kDeformerNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fnPlugin.registerCommand(
        "meshMatch",
        MeshMatchCommand::creator,
        MeshMatchCommand::createSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("meshMatch loaded");
    return status;
}


MStatus uninitializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object);
    status = fnPlugin.deregisterNode(MeshMatch::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fnPlugin.deregisterCommand("meshMatch");
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MGlobal::displayInfo("meshMatch unloaded");
    return status;
}