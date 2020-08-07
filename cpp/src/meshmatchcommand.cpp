
#include "commands.h"

#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MSyntax.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>


MeshMatchCommand::MeshMatchCommand() {}
MeshMatchCommand::~MeshMatchCommand() {}
void* MeshMatchCommand::creator() {return new MeshMatchCommand();}

MString MeshMatchCommand::name("meshMatch");

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