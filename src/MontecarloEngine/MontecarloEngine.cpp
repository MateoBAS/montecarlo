#include "MonteCarloEngine.h"
#include <thread>
#include <future>
#include <random>

RiskCalculator MonteCarloEngine::run(const Portfolio& portfolio, const SimConfig& config) {
    // 1. AHORA SÍ USAMOS LA CONFIGURACIÓN DEL USUARIO
    int numCores = config.numCores;
    
    // Fallback por si alguien llama al motor sin configurar los hilos
    if (numCores <= 0) {
        numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;
    }

    int simsPerCore = config.totalSims / numCores;

    Eigen::MatrixXd L = config.corrMatrix.llt().matrixL();
    std::vector<std::future<std::vector<double>>> futures;

    for (int i = 0; i < numCores; ++i) {
        futures.push_back(std::async(std::launch::async, MonteCarloEngine::runBatch, 
                          simsPerCore, config.totalTime, config.numSteps, 
                          std::ref(portfolio), std::ref(L), 1234 + i));
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
                                              const Portfolio& port, const Eigen::MatrixXd& L, int seed) {
    size_t numAssets = port.getNumAssets(); 
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, 1.0);
    std::vector<double> results(sims);

    // OPTIMIZACIÓN: Pre-asignamos la memoria UNA SOLA VEZ por hilo, fuera de los bucles.
    // Así evitamos que los hilos se bloqueen pidiendo memoria al Sistema Operativo.
    std::vector<std::vector<double>> Z(numAssets, std::vector<double>(steps));
    Eigen::VectorXd indep(numAssets);
    Eigen::VectorXd corr(numAssets);

    for (int i = 0; i < sims; ++i) {
        // Sobrescribimos la matriz Z en lugar de crearla de nuevo
        for (int s = 0; s < steps; ++s) {
            for (size_t a = 0; a < numAssets; ++a) {
                indep(a) = dist(gen);
            }
            
            corr = L * indep; // Eigen también recicla la memoria aquí
            
            for (size_t a = 0; a < numAssets; ++a) {
                Z[a][s] = corr(a);
            }
        }
        results[i] = port.simulatePathPnL(time, steps, Z); 
    }
    return results;
}