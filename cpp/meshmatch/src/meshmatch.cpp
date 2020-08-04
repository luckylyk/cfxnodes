
#include "meshmatch.h"

#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>


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