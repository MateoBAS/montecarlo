#include "MonteCarloEngine.h"
#include "Random/RandomGenerator.h"
#include "Random/BrownianBridge.h"
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
        futures.push_back(std::async(std::launch::async, MonteCarloEngine::runBatch, 
                                     simsPerCore, config.totalTime, config.numSteps, 
                                     std::ref(portfolio), std::ref(config.corrMatrix), i, config.rngType));
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
    int pathDimension = numAssets * steps; 

    std::vector<double> results;
    results.reserve(sims);

    // 1. Factorización de Cholesky precalculada con Eigen
    Eigen::LLT<Eigen::MatrixXd> llt(corrMatrix);
    Eigen::MatrixXd L = llt.matrixL();

    // 2. Inicialización del RNG con la dimensión total (D)
    std::unique_ptr<RandomGenerator> baseRng;
    if (rngType == RNGType::Sobol) {
        auto sobol = std::make_unique<BoostSobolGenerator>(pathDimension);
        
        unsigned long long elementosASaltar = (unsigned long long)seed * sims * pathDimension;
        sobol->skip(elementosASaltar); 
        
        baseRng = std::move(sobol);
    } else {
        // MT funciona con semillas tradicionales independientes
        baseRng = std::make_unique<MersenneTwisterGen>(1234 + seed); 
    }

    // 3. Inicializamos el Puente Browniano ANTES del bucle para no recalcular pesos
    BrownianBridge bridge(steps, time);

    // Si usamos antitéticas, el bucle da la mitad de vueltas (hace 2 simulaciones por iteración)
    int iterations = (rngType == RNGType::Antithetic) ? (sims / 2) : sims;

    for (int i = 0; i < iterations; ++i) {
        
        Eigen::MatrixXd Z_indep(numAssets, steps);
        
        // Creamos el vector con el tamaño total y tu RNG lo rellena por referencia
        std::vector<double> z_vec(pathDimension);
        baseRng->generateStandardNormals(z_vec); 
        
        // A) Llenado de matriz con o sin Puente Browniano
        if (rngType == RNGType::Sobol) {
            std::vector<double> sobol_asset(steps);
            std::vector<double> impliedZ_asset(steps);
            
            for (size_t a = 0; a < numAssets; ++a) {
                // Extraemos las dimensiones saltando de numAssets en numAssets
                // Así la dimensión 1, 2... de Sobol (las mejores) deciden el fin del año de cada activo
                for (int k = 0; k < steps; ++k) {
                    sobol_asset[k] = z_vec[a + k * numAssets];
                }
                
                // Pasamos las variables por el puente
                bridge.transformToStandardNormals(sobol_asset, impliedZ_asset);
                
                for (int s = 0; s < steps; ++s) {
                    Z_indep(a, s) = impliedZ_asset[s];
                }
            }
        } else {
            // Lógica secuencial estándar para Mersenne Twister y Antitéticas
            int idx = 0;
            for (int s = 0; s < steps; ++s) {
                for (size_t a = 0; a < numAssets; ++a) {
                    Z_indep(a, s) = z_vec[idx++];
                }
            }
        }

        // B) Correlacionar TODA la matriz de un solo golpe matricial
        Eigen::MatrixXd Z_corr = L * Z_indep;

        // C) Convertir y Simular Trayectoria Principal
        std::vector<std::vector<double>> Z_path(numAssets, std::vector<double>(steps));
        for (size_t a = 0; a < numAssets; ++a) {
            for (int s = 0; s < steps; ++s) {
                Z_path[a][s] = Z_corr(a, s);
            }
        }
        results.push_back(port.simulatePathPnL(time, steps, Z_path));

        // D) Lógica Antitética: Reflejamos la matriz Z_indep perfecta y volvemos a simular
        if (rngType == RNGType::Antithetic) {
            Eigen::MatrixXd Z_corr_anti = L * (-Z_indep); // -Z
            
            std::vector<std::vector<double>> Z_path_anti(numAssets, std::vector<double>(steps));
            for (size_t a = 0; a < numAssets; ++a) {
                for (int s = 0; s < steps; ++s) {
                    Z_path_anti[a][s] = Z_corr_anti(a, s);
                }
            }
            results.push_back(port.simulatePathPnL(time, steps, Z_path_anti));
        }
    }

    return results;
}