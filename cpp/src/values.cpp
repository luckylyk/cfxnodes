
#include "deformers.h"


double computeDistanceFactor(double const distMin, double const distMax, double dist) {
    if (dist < distMin) {
        return 0.0;}
    if (dist > distMax) {
        return 1.0;}
    double normalizeMax(distMax - distMin);
    double factor((dist - distMin) / normalizeMax);
    return factor;
}


double normalizeValue(double const inputValue, double const outputValue, double const value) {
    if (value < inputValue) {return 0.0;}
    if (value > outputValue) {return 1.0;}
    double range(1 - ((1-outputValue) + inputValue));
    double normValue(value - inputValue);
    return normValue / range;
}
