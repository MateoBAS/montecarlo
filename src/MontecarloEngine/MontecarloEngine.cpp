#include "MonteCarloEngine.h"
#include "InterestRate/InterestRateModel.h"
#include "Random/RandomGenerator.h"
#include "Random/BrownianBridge.h"
#include <thread>
#include <future>
#include <memory>
#include <stdexcept>

RiskCalculator MonteCarloEngine::run(const Portfolio& portfolio, const SimConfig& config) {
    int numCores = config.numCores;
    
    if (numCores <= 0) {
        numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;
    }

    int simsPerCore = config.totalSims / numCores;

    using MatrixRM = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    Eigen::LLT<Eigen::MatrixXd> llt(config.corrMatrix);
    MatrixRM L = llt.matrixL(); 

    std::vector<std::future<std::vector<double>>> futures;

    for (int i = 0; i < numCores; ++i) {
        futures.push_back(std::async(std::launch::async, MonteCarloEngine::runBatch, 
                                     simsPerCore, config.totalTime, config.numSteps, 
                                     std::ref(portfolio), L,
                                     config.rateModel.get(), i, config.rngType));
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
                                               const Portfolio& port, 
                                               const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& L, // Const L en RowMajor
                                               const InterestRateModel* rateModel, int seed, RNGType rngType) {
    size_t numAssets = port.getNumAssets(); 
    bool useRates = (rateModel != nullptr);
    size_t numFactors = numAssets + (useRates ? 1 : 0);
    int pathDimension = static_cast<int>(numFactors) * steps; 

    std::vector<double> results;
    results.reserve(sims);

    // 1. Inicialización del Generador de Números Aleatorios (RNG)
    std::unique_ptr<RandomGenerator> baseRng;
    if (rngType == RNGType::Sobol) {
        auto sobol = std::make_unique<BoostSobolGenerator>(pathDimension);
        unsigned long long elementosASaltar = (unsigned long long)seed * sims * pathDimension;
        sobol->skip(elementosASaltar); 
        baseRng = std::move(sobol);
    } else {
        baseRng = std::make_unique<MersenneTwisterGen>(1234 + seed); 
    }

    BrownianBridge bridge(steps, time);
    int iterations = (rngType == RNGType::Antithetic) ? (sims / 2) : sims;

    // --- HOISTING DE MEMORIA OBLIGATORIA (Estructuras críticas en la pila) ---
    std::vector<double> z_vec(pathDimension);

    // ¡CRÍTICO!: Forzamos a Eigen a usar Row-Major (Ordenación por Filas)
    // Así, cada fila (el vector temporal de un activo) es un bloque contiguo en memoria.
    using MatrixRM = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    
    MatrixRM Z_indep(static_cast<int>(numFactors), steps);
    MatrixRM Z_corr(static_cast<int>(numFactors), steps);
    
    // Matriz intermedia contigua exclusiva para la Renta Variable (Equity).
    // Esto soluciona la excepción de strides al mapear filas con Eigen::Ref en Asset.
    MatrixRM Z_equity(static_cast<int>(numAssets), steps);

    // --- GESTIÓN SELECTIVA DE MEMORIA (0 bytes en el Heap si no se usan) ---
    std::vector<double> sobol_asset;
    std::vector<double> impliedZ_asset;
    std::vector<double> Z_rate;
    std::vector<double> ratePath;
    
    // Variables específicas para el camino antitético
    std::vector<double> Z_rate_anti;
    std::vector<double> ratePathAnti;

    // Reservamos memoria únicamente si el camino lógico lo requiere
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

    // --- BUCLE PRINCIPAL ULTRA-OPTIMIZADO ---
    for (int i = 0; i < iterations; ++i) {
        // Generamos los números pseudo o cuasi aleatorios en el vector plano
        baseRng->generateStandardNormals(z_vec); 
        
        // Llenamos Z_indep respetando la disposición óptima Row-Major
        if (rngType == RNGType::Sobol) {
            for (size_t a = 0; a < numFactors; ++a) {
                for (int k = 0; k < steps; ++k) {
                    sobol_asset[k] = z_vec[a + k * numFactors]; 
                }
                
                bridge.transformToStandardNormals(sobol_asset, impliedZ_asset);
                
                for (int s = 0; s < steps; ++s) {
                    Z_indep(a, s) = impliedZ_asset[s];
                }
            }
        } else {
            int idx = 0;
            for (int s = 0; s < steps; ++s) {
                for (size_t a = 0; a < numFactors; ++a) {
                    Z_indep(a, s) = z_vec[idx++];
                }
            }
        }

        // Correlacionamos la matriz entera de un solo golpe algebraico.
        // Como L y Z_indep son RowMajor, la multiplicación es directa y óptima.
        Z_corr = L * Z_indep; 

        // Si hay modelo de tasas, extraemos su trayectoria (última fila de la matriz correlacionada)
        if (useRates) {
            int rateIndex = static_cast<int>(numFactors) - 1;
            for (int s = 0; s < steps; ++s) {
                Z_rate[s] = Z_corr(rateIndex, s);
            }
            ratePath = rateModel->simulateShortRatePath(time, steps, Z_rate);
        }

        // Extraemos en bloque únicamente las filas de los activos (omitimos la tasa si existe)
        // a nuestra matriz contigua intermedia, blindando la firma de Asset frente a excepciones.
        Z_equity = Z_corr.topRows(numAssets);

        // =====================================================================
        // TRAYECTORIA PRINCIPAL
        // =====================================================================
        results.push_back(port.simulatePathPnL(time, steps, Z_equity, ratePath, rateModel));

        // =====================================================================
        // TRAYECTORIA ANTITÉTICA
        // =====================================================================
        if (rngType == RNGType::Antithetic) {
            if (useRates) {
                int rateIndex = static_cast<int>(numFactors) - 1;
                for (int s = 0; s < steps; ++s) {
                    // Invertimos el signo de los shocks de la tasa
                    Z_rate_anti[s] = -Z_corr(rateIndex, s);
                }
                ratePathAnti = rateModel->simulateShortRatePath(time, steps, Z_rate_anti);
            }

            // Expresión perezosa de Eigen aplicada sobre la matriz empaquetada:
            // Invierte los signos al vuelo sin reservas adicionales de memoria RAM.
            results.push_back(port.simulatePathPnL(time, steps, -Z_equity, ratePathAnti, rateModel));
        }
    }

    return results;
}