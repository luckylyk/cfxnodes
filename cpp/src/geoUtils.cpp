
#include <vector>
#include <utility>

#include <maya/MItMeshVertex.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>

#include "functions.h"


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
        neightbors[index] = connections;}
    return neightbors;
}


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