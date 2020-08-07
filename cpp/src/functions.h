
#ifndef DEF_CNC_FUNCIOTNS
#define DEF_CNC_FUNCTIONS

#include <vector>
#include <utility>

std::vector<std::vector<int>> computeNeighborhoodOfVertices(MObject &mesh, int stepRadius);
double computeDistanceFactor(double const distMin, double const distMax, double dist);
double normalizeValue(double const inputValue, double const outputValue, double const value);
std::pair<unsigned int, double> getClosestVertexIndexAndDistance(MPoint const& refPoint, MObject& mesh);

#endif