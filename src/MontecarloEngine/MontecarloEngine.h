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

    // Errores estándar de VaR/ES (desactivado por defecto por coste computacional).
    bool computeStandardErrors = false;
    int bootstrapReplications = 256;
    int sobolBatchCount = 32;
};

class MonteCarloEngine {
public:
    static RiskCalculator run(const Portfolio& portfolio, const SimConfig& config);

private:
    static std::vector<double> runBatch(int sims, double time, int steps, 
                                        const Portfolio& port, const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& corrMatrix,
                                        const InterestRateModel* rateModel, int seed, RNGType rngType);
};

#endif