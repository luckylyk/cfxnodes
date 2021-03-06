// # This is a maya deformer node prototype
// # The purpose is to create a deformer able to compute the delta between two
// # geometries and apply it in the deformed one.
// # The usage is for animation/vfx pipeline based on geometries cached date
// # (whatever the format). If animation is updated and the difference has to be
// # applied on further geometry stages. For example a cloth cache. It helps to
// # avoid to redo the simulation and all the tweaks.
// # Node setup example for cloth update:
// #                        _____________
// # old animation-\       | meshDelta   |
// #                \      |-------------|
// #                 \--> *|referenceMesh|
// #                 /--> *|offsetMesh   |
// #                /      |       output|*-------> cloth sim mesh cached with
// # new animation-/       |_____________|          the old animation.


#include <maya/MFnTypedAttribute.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MDataBlock.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshVertex.h>
#include <maya/MMatrix.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MFnData.h>
#include <maya/MPoint.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include "deformers.h"


MString MeshDelta::name("meshDelta");
MTypeId MeshDelta::id(0x00035d);
MObject MeshDelta::referenceMesh;
MObject MeshDelta::offsetMesh;
MeshDelta::MeshDelta(){}
MeshDelta::~MeshDelta(){}
void* MeshDelta::creator(){return new MeshDelta();}


MStatus MeshDelta::deform(
        MDataBlock& dataBlock,
        MItGeometry& vertIter,
        const MMatrix& matrix,
        UINT multiIndex){

    float env(1 - dataBlock.inputValue(envelope).asFloat());
    MObject refMesh(dataBlock.inputValue(referenceMesh).asMesh());
    MObject offMesh(dataBlock.inputValue(offsetMesh).asMesh());

    // check if geometries are connected
    if (refMesh.isNull() || offMesh.isNull()) {
        return MS::kSuccess;}
    MItMeshVertex refMeshIt(refMesh);
    MItMeshVertex offMeshIt(offMesh);

    // check geo match
    if (refMeshIt.count() != offMeshIt.count()) {
        return MS::kSuccess;}

    if (env == 0.0) {
        return MS::kSuccess;}

    // deform
    for ( ; !vertIter.isDone(); vertIter.next()) {
        MPoint position(vertIter.position());
        MPoint refPosition(refMeshIt.position());
        MPoint offPosition(offMeshIt.position());
        float weight(weightValue(dataBlock, multiIndex, vertIter.index()));
        if (refPosition.x != offPosition.x) {
            MPoint result;
            result = position - (((refPosition - offPosition) * env) * weight);
            vertIter.setPosition(result);}
        refMeshIt.next();
        offMeshIt.next();}

    return MS::kSuccess;
}


MStatus MeshDelta::initialize(){
    MFnTypedAttribute fnTypedAttr;
    MStatus status;

    referenceMesh = fnTypedAttr.create("referenceMesh", "rm", MFnData::kMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(referenceMesh);
    attributeAffects(referenceMesh, outputGeom);

    offsetMesh = fnTypedAttr.create("offsetMesh", "om", MFnData::kMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    addAttribute(offsetMesh);
    attributeAffects(offsetMesh, outputGeom);

    return status;
}

