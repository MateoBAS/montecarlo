#include "MonteCarloEngine.h"
#include "InterestRate/InterestRateModel.h"
#include "Metrics/MonteCarloErrorPolicy.h"
#include "Random/RandomGenerator.h"
#include "Random/BrownianBridge.h"
#include <thread>
#include <future>
#include <memory>
#include <stdexcept>

namespace {

StandardErrorOptions buildStandardErrorOptions(const SimConfig& config) {
    StandardErrorOptions options;
    options.enabled = config.computeStandardErrors;
    options.bootstrapReplications = config.bootstrapReplications;
    options.sobolBatchCount = config.sobolBatchCount;

    switch (config.rngType) {
        case RNGType::MersenneTwister:
            options.scheme = ErrorSampleScheme::Independent;
            break;
        case RNGType::Antithetic:
            options.scheme = ErrorSampleScheme::AntitheticPaired;
            break;
        case RNGType::Sobol:
            options.scheme = ErrorSampleScheme::SobolBatchMeans;
            break;
    }

    return options;
}

template<int StorageOrder>
using SimMatrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, StorageOrder>;

template<int StorageOrder>
void fillIndependentShocks(SimMatrix<StorageOrder>& Z_indep,
                           const std::vector<double>& z_vec,
                           size_t numFactors,
                           int steps) {
    if constexpr (StorageOrder == Eigen::RowMajor) {
        for (size_t a = 0; a < numFactors; ++a) {
            for (int s = 0; s < steps; ++s) {
                Z_indep(static_cast<Eigen::Index>(a), s) =
                    z_vec[static_cast<size_t>(s) * numFactors + a];
            }
        }
    } else {
        for (int s = 0; s < steps; ++s) {
            for (size_t a = 0; a < numFactors; ++a) {
                Z_indep(static_cast<Eigen::Index>(a), s) =
                    z_vec[static_cast<size_t>(s) * numFactors + a];
            }
        }
    }
}

template<int StorageOrder>
void applyCorrelation(SimMatrix<StorageOrder>& Z_corr,
                      const SimMatrix<StorageOrder>& Z_indep,
                      const SimMatrix<StorageOrder>& L) {

    Z_corr.noalias() = L * Z_indep;
}

template<int StorageOrder>
void extractRateShocks(const SimMatrix<StorageOrder>& Z_corr,
                       int rateIndex,
                       int steps,
                       std::vector<double>& Z_rate) {
    if constexpr (StorageOrder == Eigen::RowMajor) {
        Eigen::Map<const Eigen::RowVectorXd> rateRow(Z_corr.row(rateIndex).data(), steps);
        Z_rate.assign(rateRow.data(), rateRow.data() + steps);
    } else {
        Z_rate.resize(steps);
        for (int s = 0; s < steps; ++s) {
            Z_rate[s] = Z_corr(rateIndex, s);
        }
    }
}

}

template<int StorageOrder>
RiskCalculator MonteCarloEngine::runWithLayout(const Portfolio& portfolio,
                                               const SimConfig& config) {
    int numCores = config.numCores;

    if (numCores <= 0) {
        numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;
    }

    const int simsPerCore = config.totalSims / numCores;

    Eigen::LLT<Eigen::MatrixXd> llt(config.corrMatrix);
    SimMatrix<StorageOrder> L = llt.matrixL();

    std::vector<std::future<std::vector<double>>> futures;

    for (int i = 0; i < numCores; ++i) {
        futures.push_back(std::async(
            std::launch::async, MonteCarloEngine::runBatchImpl<StorageOrder>,
            simsPerCore, config.totalTime, config.numSteps, std::ref(portfolio), L,
            config.rateModel.get(), i, config.rngType, config.rngSeed));
    }

    std::vector<double> allPnLs;
    allPnLs.reserve(config.totalSims);
    for (auto& f : futures) {
        auto batch = f.get();
        allPnLs.insert(allPnLs.end(), batch.begin(), batch.end());
    }

    return RiskCalculator(allPnLs, buildStandardErrorOptions(config));
}

RiskCalculator MonteCarloEngine::run(const Portfolio& portfolio, const SimConfig& config) {
    if (config.matrixLayout == MatrixStorageLayout::RowMajor) {
        return runWithLayout<Eigen::RowMajor>(portfolio, config);
    }
    return runWithLayout<Eigen::ColMajor>(portfolio, config);
}

template<int StorageOrder>
std::vector<double> MonteCarloEngine::runBatchImpl(
    int sims, double time, int steps, const Portfolio& port,
    const SimMatrix<StorageOrder>& L, const InterestRateModel* rateModel, int seed,
    RNGType rngType, int rngSeed) {
    const size_t numAssets = port.getNumAssets();
    const bool useRates = (rateModel != nullptr);
    const size_t numFactors = numAssets + (useRates ? 1 : 0);
    const int pathDimension = static_cast<int>(numFactors) * steps;

    std::vector<double> results;
    results.reserve(sims);

    const int iterations = (rngType == RNGType::Antithetic) ? sims / 2 : sims;

    std::unique_ptr<RandomGenerator> baseRng;
    if (rngType == RNGType::Sobol) {
        auto sobol = std::make_unique<BoostSobolGenerator>(pathDimension);
        unsigned long long elementosASaltar =
            static_cast<unsigned long long>(seed) * static_cast<unsigned long long>(sims) *
            static_cast<unsigned long long>(pathDimension);
        if (rngSeed != 1234) {
            elementosASaltar += static_cast<unsigned long long>(rngSeed) *
                                static_cast<unsigned long long>(pathDimension);
        }
        sobol->skip(elementosASaltar);
        baseRng = std::move(sobol);
    } else {
        baseRng = std::make_unique<MersenneTwisterGen>(rngSeed + seed);
    }

    BrownianBridge bridge(steps, time);

    std::vector<double> z_vec(pathDimension);

    SimMatrix<StorageOrder> Z_indep(static_cast<int>(numFactors), steps);
    SimMatrix<StorageOrder> Z_corr(static_cast<int>(numFactors), steps);

    std::vector<double> sobol_asset;
    std::vector<double> impliedZ_asset;
    std::vector<double> Z_rate;
    std::vector<double> ratePath;

    std::vector<double> Z_rate_anti;
    std::vector<double> ratePathAnti;

    if (rngType == RNGType::Sobol) {
        sobol_asset.resize(steps);
        impliedZ_asset.resize(steps);
    }
    if (useRates) {
        Z_rate.resize(steps);
        if (rngType == RNGType::Antithetic) {
            Z_rate_anti.resize(steps);
        }
    }

    for (int i = 0; i < iterations; ++i) {
        baseRng->generateStandardNormals(z_vec);

        if (rngType == RNGType::Sobol) {
            for (size_t a = 0; a < numFactors; ++a) {
                for (int k = 0; k < steps; ++k) {
                    sobol_asset[k] = z_vec[a + k * numFactors];
                }

                bridge.transformToStandardNormals(sobol_asset, impliedZ_asset);

                for (int s = 0; s < steps; ++s) {
                    Z_indep(static_cast<Eigen::Index>(a), s) = impliedZ_asset[s];
                }
            }
        } else {
            fillIndependentShocks(Z_indep, z_vec, numFactors, steps);
        }

        applyCorrelation(Z_corr, Z_indep, L);

        if (useRates) {
            const int rateIndex = static_cast<int>(numFactors) - 1;
            extractRateShocks(Z_corr, rateIndex, steps, Z_rate);
            ratePath = rateModel->simulateShortRatePath(time, steps, Z_rate);
        }

        const auto equityShocks = Z_corr.topRows(static_cast<Eigen::Index>(numAssets));

        const double primaryPnL =
            port.simulatePathPnL(time, steps, equityShocks, ratePath, rateModel);

        if (rngType == RNGType::Antithetic) {
            if (useRates) {
                const int rateIndex = static_cast<int>(numFactors) - 1;
                extractRateShocks(Z_corr, rateIndex, steps, Z_rate);
                for (int s = 0; s < steps; ++s) {
                    Z_rate_anti[s] = -Z_rate[s];
                }
                ratePathAnti = rateModel->simulateShortRatePath(time, steps, Z_rate_anti);
            }

            const double antitheticPnL = port.simulatePathPnL(
                time, steps, -equityShocks, ratePathAnti, rateModel);
            results.push_back(primaryPnL);
            results.push_back(antitheticPnL);
        } else {
            results.push_back(primaryPnL);
        }
    }

    return results;
}

template RiskCalculator MonteCarloEngine::runWithLayout<Eigen::RowMajor>(
    const Portfolio&, const SimConfig&);
template RiskCalculator MonteCarloEngine::runWithLayout<Eigen::ColMajor>(
    const Portfolio&, const SimConfig&);

template std::vector<double> MonteCarloEngine::runBatchImpl<Eigen::RowMajor>(
    int, double, int, const Portfolio&,
    const SimMatrix<Eigen::RowMajor>&, const InterestRateModel*, int, RNGType, int);

template std::vector<double> MonteCarloEngine::runBatchImpl<Eigen::ColMajor>(
    int, double, int, const Portfolio&,
    const SimMatrix<Eigen::ColMajor>&, const InterestRateModel*, int, RNGType, int);
