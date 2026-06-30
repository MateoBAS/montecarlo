#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <vector>
#include <random>
#include <boost/random/sobol.hpp>
#include <boost/random/uniform_01.hpp>

class RandomGenerator {
public:
    virtual ~RandomGenerator() = default;
    virtual double getStandardNormal() = 0;
    virtual void generateStandardNormals(std::vector<double>& vec) = 0;
};

class MersenneTwisterGen : public RandomGenerator {
private:
    std::mt19937 generator;
    std::normal_distribution<double> distribution;

public:
    MersenneTwisterGen(unsigned int seed);
    double getStandardNormal() override;
    void generateStandardNormals(std::vector<double>& vec) override;
};

class BoostSobolGenerator : public RandomGenerator {
private:
    boost::random::sobol sobolEngine;
    boost::random::uniform_01<double> uniformDist;

public:
    BoostSobolGenerator(std::size_t dimension);
    double getStandardNormal() override;
    void generateStandardNormals(std::vector<double>& vec) override;
    void skip(unsigned long long steps);
};

#endif 