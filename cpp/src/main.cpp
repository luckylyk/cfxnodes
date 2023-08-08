
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include "deformers.h"


MStatus initializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object, "lionelb", "0.0", "Any");
    status = fnPlugin.registerNode(
        Pooth::name,
        Pooth::id,
        Pooth::creator,
        Pooth::initialize,
        MPxDeformerNode::kDeformerNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerNode(
        Magnet::name,
        Magnet::id,
        Magnet::creator,
        Magnet::initialize,
        MPxDeformerNode::kDeformerNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerNode(
        MeshDelta::name,
        MeshDelta::id,
        MeshDelta::creator,
        MeshDelta::initialize,
        MPxDeformerNode::kDeformerNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerNode(
        PushOut::name,
        PushOut::id,
        PushOut::creator,
        PushOut::initialize,
        MPxDeformerNode::kDeformerNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo(
        "Cfx Node Collection loaded:\n"
        "\t Node registered:\n"
        "\t\tdeformers:\n"
        "\t\t\t-magnet\n"
        "\t\t\t-meshDelta\n"
        "\t\t\t-pooth\n"
        "\t\t\t-pushOut\n");
    return status;
}


MStatus uninitializePlugin(MObject object){
    MStatus status;
    MFnPlugin fnPlugin(object);

    // register nodes
    status = fnPlugin.deregisterNode(Magnet::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterNode(MeshDelta::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterNode(Pooth::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterNode(PushOut::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo(
        "Cfx Node Collection unloaded:\n"
        "\t Node unregistered:\n"
        "\t\tdeformers:\n"
        "\t\t\t-magnet\n"
        "\t\t\t-meshDelta\n"
        "\t\t\t-pooth\n"
        "\t\t\t-pushOut\n"
        "\t Command unregistered:\n"
        "\t\t\t-meshMatch\n");
    return status;
}
