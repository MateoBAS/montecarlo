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

}  // namespace

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

    return RiskCalculator(allPnLs, buildStandardErrorOptions(config));
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

    const int iterations = (rngType == RNGType::Antithetic) ? sims / 2 : sims;

    // 1. Inicialización del Generador de Números Aleatorios (RNG)
    std::unique_ptr<RandomGenerator> baseRng;
    if (rngType == RNGType::Sobol) {
        auto sobol = std::make_unique<BoostSobolGenerator>(pathDimension);
        unsigned long long elementosASaltar =
            static_cast<unsigned long long>(seed) * static_cast<unsigned long long>(sims) *
            static_cast<unsigned long long>(pathDimension);
        sobol->skip(elementosASaltar);
        baseRng = std::move(sobol);
    } else {
        baseRng = std::make_unique<MersenneTwisterGen>(1234 + seed);
    }

    BrownianBridge bridge(steps, time);

    // --- HOISTING DE MEMORIA OBLIGATORIA (Estructuras críticas en la pila) ---
    std::vector<double> z_vec(pathDimension);

    // ¡CRÍTICO!: Forzamos a Eigen a usar Row-Major (Ordenación por Filas)
    // Así, cada fila (el vector temporal de un activo) es un bloque contiguo en memoria.
    using MatrixRM = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    
    MatrixRM Z_indep(static_cast<int>(numFactors), steps);
    MatrixRM Z_corr(static_cast<int>(numFactors), steps);

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

        // Vista sobre las filas de activos (RowMajor → cada fila es contigua en memoria).
        auto Z_equity = Z_corr.topRows(static_cast<Eigen::Index>(numAssets));

        const double primaryPnL =
            port.simulatePathPnL(time, steps, Z_equity, ratePath, rateModel);

        if (rngType == RNGType::Antithetic) {
            if (useRates) {
                int rateIndex = static_cast<int>(numFactors) - 1;
                for (int s = 0; s < steps; ++s) {
                    Z_rate_anti[s] = -Z_corr(rateIndex, s);
                }
                ratePathAnti = rateModel->simulateShortRatePath(time, steps, Z_rate_anti);
            }

            const double antitheticPnL =
                port.simulatePathPnL(time, steps, -Z_equity, ratePathAnti, rateModel);
            // VaR/ES requieren la cola empírica completa; promediar pares subestima ~×2 el riesgo.
            results.push_back(primaryPnL);
            results.push_back(antitheticPnL);
        } else {
            results.push_back(primaryPnL);
        }
    }

    return results;
}