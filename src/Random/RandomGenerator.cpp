#include "RandomGenerator.h"
#include "Maths/MathUtils.h"

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

BoostSobolGenerator::BoostSobolGenerator(std::size_t dimension) 
    : sobolEngine(dimension) {}

double BoostSobolGenerator::getStandardNormal() {

    double val = static_cast<double>(sobolEngine());
    double max_val = static_cast<double>((sobolEngine.max)());
    double u = val / max_val;
    
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