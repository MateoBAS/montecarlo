#include "RandomGenerator.h"
#include "MathUtils.h"

MersenneTwisterGen::MersenneTwisterGen(unsigned int seed) 
    : generator(seed), distribution(0.0, 1.0) {}

double MersenneTwisterGen::getStandardNormal() {
    return distribution(generator);
}

void MersenneTwisterGen::generateStandardNormals(std::vector<double>& vec) {
    for (double& v : vec) {
        v = getStandardNormal();
    }
}

AntitheticGenerator::AntitheticGenerator(RandomGenerator* innerGen) 
    : innerGenerator(innerGen), returnNegatedNext(false), lastGeneratedNormal(0.0) {}

double AntitheticGenerator::getStandardNormal() {
    if (returnNegatedNext) {
        returnNegatedNext = false;
        return -lastGeneratedNormal; 
    } else {
        lastGeneratedNormal = innerGenerator->getStandardNormal();
        returnNegatedNext = true;
        return lastGeneratedNormal;
    }
}

void AntitheticGenerator::generateStandardNormals(std::vector<double>& vec) {
    for (double& v : vec) {
        v = getStandardNormal();
    }
}

BoostSobolGenerator::BoostSobolGenerator(std::size_t dimension) 
    : sobolEngine(dimension) {}

double BoostSobolGenerator::getStandardNormal() {
    double u = uniformDist(sobolEngine);
    if (u <= 0.0) u = 1e-9;
    if (u >= 1.0) u = 1.0 - 1e-9;
    return MathUtils::inverseNormalCDF(u);
}

void BoostSobolGenerator::generateStandardNormals(std::vector<double>& vec) {
    for (double& v : vec) {
        v = getStandardNormal();
    }
}

void BoostSobolGenerator::skip(unsigned long long steps) {
    sobolEngine.discard(steps);
}