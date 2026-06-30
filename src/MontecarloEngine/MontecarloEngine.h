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

enum class MatrixStorageLayout {
    RowMajor,
    ColMajor
};

struct SimConfig {
    int totalSims;
    double totalTime;
    int numSteps;
    Eigen::MatrixXd corrMatrix;
    int numCores;
    RNGType rngType = RNGType::MersenneTwister;
    std::shared_ptr<InterestRateModel> rateModel = nullptr;

    bool computeStandardErrors = false;
    int bootstrapReplications = 256;
    int sobolBatchCount = 32;

    int rngSeed = 1234;

    MatrixStorageLayout matrixLayout = MatrixStorageLayout::RowMajor;
};

class MonteCarloEngine {
public:
    static RiskCalculator run(const Portfolio& portfolio, const SimConfig& config);

private:
    template<int StorageOrder>
    static RiskCalculator runWithLayout(const Portfolio& portfolio, const SimConfig& config);

    template<int StorageOrder>
    static std::vector<double> runBatchImpl(
        int sims, double time, int steps,
        const Portfolio& port,
        const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, StorageOrder>& L,
        const InterestRateModel* rateModel, int seed, RNGType rngType, int rngSeed);
};

#endif
