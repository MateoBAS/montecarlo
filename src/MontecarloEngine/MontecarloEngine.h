#ifndef MONTE_CARLO_ENGINE_H
#define MONTE_CARLO_ENGINE_H

#include <vector>
#include <memory>
#include <Eigen/Dense>
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"

class InterestRateModel;

enum class RNGType {
    MersenneTwister,
    Antithetic,
    Sobol
};

struct SimConfig {
    int totalSims;
    double totalTime;
    int numSteps;
    Eigen::MatrixXd corrMatrix;
    int numCores;
    RNGType rngType = RNGType::MersenneTwister;
    std::shared_ptr<InterestRateModel> rateModel = nullptr;
};

class MonteCarloEngine {
public:
    static RiskCalculator run(const Portfolio& portfolio, const SimConfig& config);

private:
    // Cambiamos Eigen::MatrixXd& L por corrMatrix para poder usar tu CorrelatedGenerator
    static std::vector<double> runBatch(int sims, double time, int steps, 
                                        const Portfolio& port, const Eigen::MatrixXd& corrMatrix,
                                        const InterestRateModel* rateModel, int seed, RNGType rngType);
};

#endif