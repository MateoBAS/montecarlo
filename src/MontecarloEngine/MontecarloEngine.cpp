#include "MonteCarloEngine.h"
#include "Random/RandomGenerator.h"
#include "Random/CorrelatedGenerator.h"
#include <thread>
#include <future>
#include <memory>

RiskCalculator MonteCarloEngine::run(const Portfolio& portfolio, const SimConfig& config) {
    int numCores = config.numCores;
    
    if (numCores <= 0) {
        numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;
    }

    int simsPerCore = config.totalSims / numCores;
    std::vector<std::future<std::vector<double>>> futures;

    for (int i = 0; i < numCores; ++i) {
        // Pasamos la matriz de correlación original y el tipo de RNG
        futures.push_back(std::async(std::launch::async, MonteCarloEngine::runBatch, 
                          simsPerCore, config.totalTime, config.numSteps, 
                          std::ref(portfolio), std::ref(config.corrMatrix), 1234 + i, config.rngType));
    }

    std::vector<double> allPnLs;
    allPnLs.reserve(config.totalSims);
    for (auto& f : futures) {
        auto batch = f.get();
        allPnLs.insert(allPnLs.end(), batch.begin(), batch.end());
    }

    return RiskCalculator(allPnLs);
}

std::vector<double> MonteCarloEngine::runBatch(int sims, double time, int steps, 
                                               const Portfolio& port, const Eigen::MatrixXd& corrMatrix, 
                                               int seed, RNGType rngType) {
    size_t numAssets = port.getNumAssets(); 
    std::vector<double> results(sims);

    // 1. INYECCIÓN DE DEPENDENCIAS Y CONSTRUCCIÓN DE LA JERARQUÍA
    std::unique_ptr<RandomGenerator> baseRng;
    std::unique_ptr<RandomGenerator> antitheticWrapper; 

    if (rngType == RNGType::Sobol) {
        auto sobol = std::make_unique<BoostSobolGenerator>(numAssets);
        // CRÍTICO en concurrencia: Hacemos skip para que el Hilo 2 no empiece con 
        // la misma secuencia Sobol que el Hilo 1.
        sobol->skip(seed * sims * steps); 
        baseRng = std::move(sobol);
    } else {
        baseRng = std::make_unique<MersenneTwisterGen>(seed);
        if (rngType == RNGType::Antithetic) {
            antitheticWrapper = std::make_unique<AntitheticGenerator>(baseRng.get());
        }
    }

    RandomGenerator* activeRng = (rngType == RNGType::Antithetic) ? antitheticWrapper.get() : baseRng.get();

    CorrelatedGenerator corrGen(activeRng, corrMatrix);

    std::vector<std::vector<double>> Z(numAssets, std::vector<double>(steps));

    for (int i = 0; i < sims; ++i) {
        for (int s = 0; s < steps; ++s) {
            // Tu clase se encarga de todo el Álgebra Lineal (Cholesky)
            std::vector<double> correlatedZs = corrGen.getCorrelatedNormals();
            
            for (size_t a = 0; a < numAssets; ++a) {
                Z[a][s] = correlatedZs[a];
            }
        }
        results[i] = port.simulatePathPnL(time, steps, Z); 
    }
    return results;
}