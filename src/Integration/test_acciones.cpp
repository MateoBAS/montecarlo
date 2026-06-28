#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <memory>
#include <numeric>
#include <cmath>
#include <thread>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <Eigen/Dense>

#include "MontecarloEngine/MontecarloEngine.h"
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"
#include "Asset/GBMAsset.h"
#include "Asset/EuropeanOption.h"
#include "Asset/BarrierOption.h"
#include "Asset/CouponBond.h"
#include "InterestRate/YieldCurve.h"
#include "InterestRate/HullWhiteModel.h"

// ============================================================================
// SimConfig — referencia rápida (MontecarloEngine.h)
// ============================================================================
// totalSims   : número de trayectorias Monte Carlo
// totalTime   : horizonte de simulación (años)
// numSteps    : pasos temporales (dt = totalTime / numSteps)
// numCores    : hilos para paralelizar batches
// rngType     : MersenneTwister | Antithetic | Sobol
// rateModel   : nullptr → sin dinámica de tipos (drift fijo, sin descuento HW)
//               shared_ptr<HullWhiteModel> → simula ratePath, acopla drift y descuenta
// corrMatrix  : sin tipos → numActivos × numActivos
//               con tipos  → (numActivos + 1) × (numActivos + 1), última fila/col = factor tasa
// computeStandardErrors : false → solo VaR/ES puntual; true → adjunta error estándar
// bootstrapReplications: réplicas bootstrap para ES (MT/Antithetic; esquema CLT)
// sobolBatchCount      : nº de lotes para error en Sobol (varianza entre lotes)
//   Esquema automático según rngType:
//   - MersenneTwister → bootstrap i.i.d. sobre N muestras
//   - Antithetic      → N trayectorias primarias + N antitéticas (2N PnL en bruto);
//                       bootstrap por pares (Z,-Z) para el error
//   - Sobol           → error por lotes (no CLT i.i.d.)
// matrixLayout: RowMajor (defecto) | ColMajor — disposición de Z_indep/Z_corr en runBatch
// rngSeed       : semilla base del generador
// ============================================================================

namespace {

struct CarteraBaseCompleta {
    std::shared_ptr<HullWhiteModel> hw;
    Portfolio portfolio;
    Eigen::MatrixXd corrMatrix;
    double initialValue = 0.0;
};

CarteraBaseCompleta makeCarteraBaseCompleta(double quantityScale = 1.0) {
    // Curva inicial y dinámica Hull-White compartida por el bono y el descuento global
    YieldCurve curve({0.5, 1.0, 2.0, 5.0, 10.0}, {0.02, 0.022, 0.025, 0.03, 0.032});
    auto hw = std::make_shared<HullWhiteModel>(0.05, 0.01, curve);

    CarteraBaseCompleta spec;
    spec.hw = hw;

    // --- Activos disponibles en el motor (4 tipos) ---
    // 1) Spot equity (GBM)
    auto equity = std::make_shared<GBMAsset>("EquityIndex", 100.0, 0.02, 0.20);
    // 2) Opción europea (call OTM sobre subyacente tech correlacionado)
    auto euroCall = std::make_shared<EuropeanOption>(
        "EuroCall_Tech", 12.0, 100.0, 105.0, 0.02, 0.25, OptionType::Call);
    // 3) Opción barrera up-and-out (subyacente industrial, vol algo mayor)
    auto barrierCall = std::make_shared<BarrierOption>(
        "BarrierCall_Ind", 8.0, 100.0, 100.0, 130.0, 0.02, 0.28);
    // 4) Bono cupón con valoración HW (cupones semestrales implícitos vía tasa anual)
    auto govBond = std::make_shared<CouponBond>("GovBond_HW", 1000.0, 0.03, 5.0, hw);

    // Notional ~5k por línea (cartera diversificada multi-activo)
    spec.portfolio.addPosition(equity, 50.0 * quantityScale);
    spec.portfolio.addPosition(euroCall, 400.0 * quantityScale);
    spec.portfolio.addPosition(barrierCall, 500.0 * quantityScale);
    spec.portfolio.addPosition(govBond, 5.0 * quantityScale);

    spec.initialValue = spec.portfolio.getInitialValue();

    // Correlación (numActivos + 1): [Equity, EuroCall, Barrier, Bond, Rate]
    // - Equities/derivados: alta correlación cruzada (mismo régimen de mercado)
    // - Bonos vs equity: correlación negativa moderada (flight-to-quality)
    // - Todos los risky vs tasa: correlación positiva moderada (macro)
    // - Bono vs tasa: la más alta (duration / reinversión)
    spec.corrMatrix.resize(5, 5);
    spec.corrMatrix <<
        1.00,  0.80,  0.60, -0.20,  0.25,
        0.80,  1.00,  0.55, -0.15,  0.22,
        0.60,  0.55,  1.00, -0.12,  0.18,
       -0.20, -0.15, -0.12,  1.00,  0.45,
        0.25,  0.22,  0.18,  0.45,  1.00;

    return spec;
}

// Réplicas del bloque benchmark1: cada réplica añade 4 activos distintos (mismos parámetros).
// La correlación intra-réplica replica el bloque 4×4 base; entre réplicas se escala ×0.75.
CarteraBaseCompleta makeCarteraBenchmarkReplicada(int numReplicas) {
    if (numReplicas < 1) {
        throw std::invalid_argument("makeCarteraBenchmarkReplicada: numReplicas >= 1");
    }

    YieldCurve curve({0.5, 1.0, 2.0, 5.0, 10.0}, {0.02, 0.022, 0.025, 0.03, 0.032});
    auto hw = std::make_shared<HullWhiteModel>(0.05, 0.01, curve);

    CarteraBaseCompleta spec;
    spec.hw = hw;

    constexpr int kAssetsPerReplica = 4;
    constexpr double kCrossReplicaScale = 0.75;

    const Eigen::Matrix4d riskyBlock =
        (Eigen::Matrix4d() << 1.00, 0.80, 0.60, -0.20,
                             0.80, 1.00, 0.55, -0.15,
                             0.60, 0.55, 1.00, -0.12,
                            -0.20, -0.15, -0.12, 1.00)
            .finished();
    const Eigen::Vector4d riskyToRate(0.25, 0.22, 0.18, 0.45);

    for (int replica = 0; replica < numReplicas; ++replica) {
        const std::string suffix = "_r" + std::to_string(replica);

        auto equity = std::make_shared<GBMAsset>(
            "EquityIndex" + suffix, 100.0, 0.02, 0.20);
        auto euroCall = std::make_shared<EuropeanOption>(
            "EuroCall_Tech" + suffix, 12.0, 100.0, 105.0, 0.02, 0.25, OptionType::Call);
        auto barrierCall = std::make_shared<BarrierOption>(
            "BarrierCall_Ind" + suffix, 8.0, 100.0, 100.0, 130.0, 0.02, 0.28);
        auto govBond = std::make_shared<CouponBond>(
            "GovBond_HW" + suffix, 1000.0, 0.03, 5.0, hw);

        spec.portfolio.addPosition(equity, 50.0);
        spec.portfolio.addPosition(euroCall, 400.0);
        spec.portfolio.addPosition(barrierCall, 500.0);
        spec.portfolio.addPosition(govBond, 5.0);
    }

    const int numAssets = kAssetsPerReplica * numReplicas;
    const int dim = numAssets + 1;
    spec.corrMatrix.resize(dim, dim);
    spec.corrMatrix.setZero();

    for (int replicaI = 0; replicaI < numReplicas; ++replicaI) {
        for (int replicaJ = replicaI; replicaJ < numReplicas; ++replicaJ) {
            const double crossScale =
                (replicaI == replicaJ) ? 1.0 : kCrossReplicaScale;
            for (int typeI = 0; typeI < kAssetsPerReplica; ++typeI) {
                for (int typeJ = (replicaI == replicaJ ? typeI : 0);
                     typeJ < kAssetsPerReplica; ++typeJ) {
                    const int i = replicaI * kAssetsPerReplica + typeI;
                    const int j = replicaJ * kAssetsPerReplica + typeJ;
                    const double rho = crossScale * riskyBlock(typeI, typeJ);
                    spec.corrMatrix(i, j) = rho;
                    spec.corrMatrix(j, i) = rho;
                }
            }
        }
    }
    for (int i = 0; i < numAssets; ++i) {
        spec.corrMatrix(i, numAssets) = riskyToRate(i % kAssetsPerReplica);
        spec.corrMatrix(numAssets, i) = riskyToRate(i % kAssetsPerReplica);
    }
    spec.corrMatrix(numAssets, numAssets) = 1.0;

    spec.initialValue = spec.portfolio.getInitialValue();
    return spec;
}

int simsForLayoutBenchmark(int numAssets) {
    if (numAssets >= 8192) {
        return 20;
    }
    if (numAssets >= 4096) {
        return 30;
    }
    if (numAssets >= 2048) {
        return 50;
    }
    return std::max(100, 64000 / std::max(numAssets, 1));
}

int layoutBenchmarkRepetitions(int numAssets) {
    if (numAssets >= 4096) {
        return 1;
    }
    if (numAssets >= 2048) {
        return 2;
    }
    return 3;
}

int layoutBenchmarkNumCores() {
    return 1;
}

struct AmdahlScalingParams {
    int totalSims = 100000;
    int medidasPorPunto = 1000;
    int warmupIterations = 25;
};

// Presupuesto conservador (<8 h): 6 carteras × 6 hilos, medidas adaptativas.
AmdahlScalingParams amdahlScalingParamsForAssets(int numAssets) {
    AmdahlScalingParams p;
    if (numAssets <= 16) {
        p.totalSims = 40000;
        p.medidasPorPunto = 250;
        p.warmupIterations = 8;
    } else if (numAssets <= 32) {
        p.totalSims = 30000;
        p.medidasPorPunto = 250;
        p.warmupIterations = 8;
    } else if (numAssets <= 64) {
        p.totalSims = 20000;
        p.medidasPorPunto = 200;
        p.warmupIterations = 5;
    } else if (numAssets <= 256) {
        p.totalSims = 8000;
        p.medidasPorPunto = 150;
        p.warmupIterations = 5;
    } else {
        p.totalSims = 5000;
        p.medidasPorPunto = 120;
        p.warmupIterations = 3;
    }
    return p;
}

// Mismos 6 puntos en todas las carteras: densidad cerca del plateau (9, 12, 16).
std::vector<int> amdahlHilosToMeasure(int maxHilos) {
    const std::vector<int> candidates = {1, 3, 6, 9, 12, 16};

    std::vector<int> hilos;
    hilos.reserve(candidates.size());
    for (int k : candidates) {
        if (k <= maxHilos) {
            hilos.push_back(k);
        }
    }
    if (hilos.empty() || hilos.back() < maxHilos) {
        hilos.push_back(maxHilos);
    }
    return hilos;
}

void runAmdahlTimingSweepForPortfolio(
    const CarteraBaseCompleta& bench,
    SimConfig config,
    const std::vector<int>& hilosAMedir,
    const AmdahlScalingParams& params,
    int numReplicas,
    int numAssets,
    std::ofstream& archivo) {
    const Portfolio& cartera = bench.portfolio;

    config.totalSims = params.totalSims;
    config.rateModel = bench.hw;
    config.corrMatrix = bench.corrMatrix;

    std::cout << "  N=" << config.totalSims
              << ", medidas/hilo=" << params.medidasPorPunto
              << ", warm-up=" << params.warmupIterations
              << ", hilos={";
    for (size_t i = 0; i < hilosAMedir.size(); ++i) {
        if (i > 0) {
            std::cout << ',';
        }
        std::cout << hilosAMedir[i];
    }
    std::cout << "}\n";
    std::cout.flush();

    Eigen::LLT<Eigen::MatrixXd> llt(bench.corrMatrix);
    if (llt.info() != Eigen::Success) {
        throw std::runtime_error(
            "Matriz de correlacion no definida positiva (replicas=" + std::to_string(numReplicas) + ")");
    }

    for (int hilos : hilosAMedir) {
        config.numCores = hilos;

        std::cout << "  >>> Hilos=" << hilos << " — warm-up...\n";
        std::cout.flush();
        for (int w = 0; w < params.warmupIterations; ++w) {
            MonteCarloEngine::run(cartera, config);
        }

        std::cout << "  >>> Hilos=" << hilos << " — midiendo " << params.medidasPorPunto
                  << " ejecuciones...\n";
        std::cout.flush();

        std::vector<double> tiempos(static_cast<size_t>(params.medidasPorPunto));
        for (int m = 0; m < params.medidasPorPunto; ++m) {
            const auto inicio = std::chrono::high_resolution_clock::now();
            const RiskCalculator report = MonteCarloEngine::run(cartera, config);
            volatile double var = report.calculateVaR(0.99);
            const auto fin = std::chrono::high_resolution_clock::now();
            tiempos[static_cast<size_t>(m)] =
                std::chrono::duration<double, std::milli>(fin - inicio).count();

            if ((m + 1) % 50 == 0) {
                std::cout << "      progreso: " << (m + 1) << "/" << params.medidasPorPunto << '\n';
                std::cout.flush();
            }
        }

        const double sumaTiempos = std::accumulate(tiempos.begin(), tiempos.end(), 0.0);
        const double media = sumaTiempos / params.medidasPorPunto;

        double sumaDiferenciasCuadrado = 0.0;
        for (double t : tiempos) {
            sumaDiferenciasCuadrado += (t - media) * (t - media);
        }
        const double varianza =
            sumaDiferenciasCuadrado / std::max(params.medidasPorPunto - 1, 1);
        const double desviacionEstandar = std::sqrt(varianza);

        archivo << numReplicas << ',' << numAssets << ',' << config.numSteps << ','
                << config.totalSims << ',' << hilos << ',' << media << ','
                << desviacionEstandar << '\n';
        archivo.flush();

        std::cout << "  Hilos: " << hilos << " | T_Medio: " << media << " ms"
                  << " | StdDev: +/-" << desviacionEstandar << " ms\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

struct LayoutBenchmarkResult {
    double elapsedMs = 0.0;
    double var99 = 0.0;
};

LayoutBenchmarkResult runLayoutBenchmark(const Portfolio& portfolio, SimConfig config,
                                         MatrixStorageLayout layout) {
    config.matrixLayout = layout;
    config.computeStandardErrors = false;

    const auto t0 = std::chrono::high_resolution_clock::now();
    const RiskCalculator report = MonteCarloEngine::run(portfolio, config);
    const auto t1 = std::chrono::high_resolution_clock::now();

    LayoutBenchmarkResult result;
    result.elapsedMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    result.var99 = report.calculateVaR(0.99);
    return result;
}

struct LayoutBenchmarkPair {
    LayoutBenchmarkResult row;
    LayoutBenchmarkResult col;
    double speedup = 0.0;
};

LayoutBenchmarkPair runLayoutBenchmarkPair(const Portfolio& portfolio, SimConfig config,
                                           int rngSeed) {
    config.rngSeed = rngSeed;
    LayoutBenchmarkPair pair;
    pair.row = runLayoutBenchmark(portfolio, config, MatrixStorageLayout::RowMajor);
    pair.col = runLayoutBenchmark(portfolio, config, MatrixStorageLayout::ColMajor);
    pair.speedup = pair.row.elapsedMs > 0.0 ? pair.col.elapsedMs / pair.row.elapsedMs : 0.0;
    return pair;
}

LayoutBenchmarkPair runLayoutBenchmarkMedianPair(const Portfolio& portfolio, SimConfig config,
                                                 int repetitions = 3) {
    std::vector<double> speedups;
    speedups.reserve(static_cast<size_t>(repetitions));

    LayoutBenchmarkPair last;
    for (int rep = 0; rep < repetitions; ++rep) {
        last = runLayoutBenchmarkPair(portfolio, config, 1234 + rep);
        speedups.push_back(last.speedup);
    }

    const size_t mid = speedups.size() / 2;
    std::nth_element(speedups.begin(), speedups.begin() + static_cast<std::ptrdiff_t>(mid),
                     speedups.end());
    const double target = speedups[mid];

    for (int rep = 0; rep < repetitions; ++rep) {
        const LayoutBenchmarkPair trial = runLayoutBenchmarkPair(portfolio, config, 1234 + rep);
        if (std::abs(trial.speedup - target) < 1e-9) {
            return trial;
        }
    }
    return last;
}

void writeCsvValue(std::ofstream& out, double value) {
    if (std::isfinite(value)) {
        out << value;
    }
}

void writeCsvEstimate(std::ofstream& out, const RiskEstimate& est) {
    writeCsvValue(out, est.value);
    out << ',';
    if (est.hasStandardError()) {
        writeCsvValue(out, est.standardError);
    }
    out << ',';
}

void appendConvergenceRow(std::ofstream& out, int numSims, const char* rngLabel,
                          const RiskCalculator& report) {
    out << numSims << ',' << rngLabel << ',';
    writeCsvEstimate(out, report.estimateMeanPnL());
    writeCsvEstimate(out, report.estimateVaR(0.95));
    writeCsvEstimate(out, report.estimateES(0.95));
    writeCsvEstimate(out, report.estimateVaR(0.99));
    const RiskEstimate es975 = report.estimateES(0.975);
    writeCsvValue(out, es975.value);
    out << ',';
    if (es975.hasStandardError()) {
        writeCsvValue(out, es975.standardError);
    }
    out << '\n';
}

}  // namespace

// ============================================================================

BOOST_AUTO_TEST_SUITE(Escenario_Rendimiento_Estadistico)

BOOST_AUTO_TEST_CASE(Amdahl_Con_Incertidumbre) {
    std::cout << "--> Iniciando benchmark estadístico de Amdahl (benchmark1)...\n";

    const CarteraBaseCompleta base = makeCarteraBaseCompleta();
    const Portfolio& cartera = base.portfolio;

    SimConfig config;
    config.totalSims = 100000;
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = 1;  // Sobrescrito en el bucle de hilos (1 .. maxHilos)
    config.rngType = RNGType::MersenneTwister;
    config.rngSeed = 1234;
    config.rateModel = base.hw;
    config.corrMatrix = base.corrMatrix;
    config.computeStandardErrors = false;
    config.bootstrapReplications = 256;
    config.sobolBatchCount = 32;

    const int maxHilos = std::thread::hardware_concurrency();
    if (maxHilos <= 0) {
        BOOST_FAIL("No se pudo determinar hardware_concurrency()");
    }

    const int medidasPorPunto = 1000;
    const int iteracionesCalentamiento = 25;

    const std::string rutaCSV = "../graphics/amdahl/amdahl_estadistico_benchmark1.csv";
    std::ofstream archivo(rutaCSV);

    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/amdahl/amdahl_estadistico_benchmark1.csv");
    }

    archivo << std::setprecision(12);
    archivo << "Hilos,Tiempo_Medio_ms,Desviacion_ms\n";

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Cartera: benchmark1 (V0=" << base.initialValue
              << ", activos=" << cartera.getNumAssets() << ")\n";
    std::cout << "SimConfig: N=" << config.totalSims
              << ", T=" << config.totalTime << " año, pasos=" << config.numSteps
              << ", MT, rngSeed=" << config.rngSeed << ", Hull-White activo\n";
    std::cout << "Hilos a medir: 1.." << maxHilos << "\n";
    std::cout << "Mediciones por punto: " << medidasPorPunto
              << " | Warm-up: " << iteracionesCalentamiento << " iteraciones\n";
    std::cout << "Salida: " << rutaCSV << "\n";
    std::cout << "IMPORTANTE: cierra otros programas antes de ejecutar este test.\n\n";

    for (int hilos = 1; hilos <= maxHilos; ++hilos) {
        config.numCores = hilos;

        std::cout << ">>> Hilos=" << hilos << " — warm-up (" << iteracionesCalentamiento
                  << " iteraciones)...\n";
        std::cout.flush();

        for (int w = 0; w < iteracionesCalentamiento; ++w) {
            MonteCarloEngine::run(cartera, config);
        }

        std::cout << ">>> Hilos=" << hilos << " — midiendo " << medidasPorPunto
                  << " ejecuciones...\n";
        std::cout.flush();

        std::vector<double> tiempos(medidasPorPunto);
        for (int m = 0; m < medidasPorPunto; ++m) {
            const auto inicio = std::chrono::high_resolution_clock::now();

            const RiskCalculator report = MonteCarloEngine::run(cartera, config);
            volatile double var = report.calculateVaR(0.99);

            const auto fin = std::chrono::high_resolution_clock::now();
            tiempos[m] = std::chrono::duration<double, std::milli>(fin - inicio).count();

            if ((m + 1) % 100 == 0) {
                std::cout << "    progreso: " << (m + 1) << "/" << medidasPorPunto << "\n";
                std::cout.flush();
            }
        }

        const double sumaTiempos = std::accumulate(tiempos.begin(), tiempos.end(), 0.0);
        const double media = sumaTiempos / medidasPorPunto;

        double sumaDiferenciasCuadrado = 0.0;
        for (double t : tiempos) {
            sumaDiferenciasCuadrado += (t - media) * (t - media);
        }
        const double varianza = sumaDiferenciasCuadrado / (medidasPorPunto - 1);
        const double desviacionEstandar = std::sqrt(varianza);

        archivo << hilos << ',' << media << ',' << desviacionEstandar << '\n';
        archivo.flush();

        std::cout << "Hilos: " << hilos
                  << " | T_Medio: " << media << " ms"
                  << " | StdDev: +/-" << desviacionEstandar << " ms\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    archivo.close();
    std::cout << "--> Datos guardados en " << rutaCSV << '\n';
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(Amdahl_Con_Incertidumbre_Cartera_Grande) {
    std::cout << "--> Amdahl vs. escala de cartera (benchmark1 replicada)...\n";
    std::cout << "    Objetivo: tiempos por (activos, hilos) para estimar P en Python.\n";
    std::cout << "    Presupuesto: tope 8 h; 6 carteras x 6 hilos fijos.\n\n";

    // 1 replica = benchmark1 (4 activos): amdahl_estadistico_benchmark1.csv
    const std::vector<int> replicaCounts = {
        2, 4, 8, 16, 64, 128,
    };

    const int maxHilos = std::thread::hardware_concurrency();
    if (maxHilos <= 0) {
        BOOST_FAIL("No se pudo determinar hardware_concurrency()");
    }

    SimConfig config;
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = 1;
    config.rngType = RNGType::MersenneTwister;
    config.rngSeed = 1234;
    config.computeStandardErrors = false;
    config.bootstrapReplications = 256;
    config.sobolBatchCount = 32;

    const std::string rutaCSV = "../graphics/amdahl/amdahl_escala_activos.csv";
    std::ofstream archivo(rutaCSV);
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/amdahl/amdahl_escala_activos.csv");
    }

    archivo << std::setprecision(12);
    archivo << "NumReplicas,NumAssets,NumSteps,N,Hilos,Tiempo_Medio_ms,Desviacion_ms\n";

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "maxHilos=" << maxHilos << " | hilos fijos: {1, 3, 6, 9, 12, 16}\n";
    std::cout << "Salida: " << rutaCSV << '\n';
    std::cout << "IMPORTANTE: cierra otros programas antes de ejecutar este test.\n\n";

    for (int replicas : replicaCounts) {
        std::cout << ">>> Construyendo cartera (" << replicas << " replicas)...\n";
        std::cout.flush();

        const CarteraBaseCompleta bench = makeCarteraBenchmarkReplicada(replicas);
        const int numAssets = static_cast<int>(bench.portfolio.getNumAssets());
        const AmdahlScalingParams params = amdahlScalingParamsForAssets(numAssets);
        const std::vector<int> hilosAMedir = amdahlHilosToMeasure(maxHilos);

        std::cout << ">>> Midiendo replicas=" << replicas
                  << " | activos=" << numAssets
                  << " | corr=" << bench.corrMatrix.rows() << "x"
                  << bench.corrMatrix.cols() << '\n';

        runAmdahlTimingSweepForPortfolio(
            bench, config, hilosAMedir, params, replicas, numAssets, archivo);
    }

    archivo.close();
    std::cout << "--> Datos guardados en " << rutaCSV << '\n';
    std::cout << "    Combinar con amdahl_estadistico_benchmark1.csv (1 replica) para P vs. activos.\n";
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

// ========================================================================
// NUEVA SUITE: ESTUDIO DE CONVERGENCIA DE GENERADORES (VRT)
// ========================================================================

BOOST_AUTO_TEST_SUITE(Escenario_Convergencia_RNG)

BOOST_AUTO_TEST_CASE(Comparativa_Generadores) {
    std::cout << "\n--> Iniciando Benchmark de Convergencia de RNGs...\n";
    
    // 1. Configuración de la cartera
    Portfolio cartera;

    auto stockA = std::make_shared<GBMAsset>("TechA", 150.0, 0.05, 0.25);
    auto stockB = std::make_shared<GBMAsset>("TechB", 200.0, 0.08, 0.30);
    
    cartera.addPosition(stockA, 100.0);
    cartera.addPosition(stockB, 50.0);

    // 2. Configuración base del motor (todos los campos explícitos)
    SimConfig config;
    config.totalSims = 128;            // Sobrescrito en el bucle (128 .. 2^20)
    config.totalTime = 1.0;            // 1 año de proyección
    config.numSteps = 252;             // Simulación diaria
    config.numCores = static_cast<int>(std::thread::hardware_concurrency());
    config.rngType = RNGType::MersenneTwister;  // Sobrescrito por iteración (MT / Antithetic / Sobol)
    config.rateModel = nullptr;        // Sin Hull-White
    config.computeStandardErrors = false;  // Convergencia pesada (hasta 2^20 sims): sin errores MC
    config.bootstrapReplications = 256;    // Activar junto con computeStandardErrors=true
    config.sobolBatchCount = 32;           // Activar junto con computeStandardErrors=true (Sobol)

    // Correlación equity-equity (2 activos, sin factor de tipos)
    Eigen::MatrixXd corr(2, 2);
    corr << 1.0, 0.4,
            0.4, 1.0;
    config.corrMatrix = corr;

    // 3. Preparación del archivo de salida
    std::string rutaCSV = "../graphics/convergencia/convergencia_rng.csv";
    std::ofstream archivo(rutaCSV);
    
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV para la convergencia en ../graphics/convergencia/");
    }
    
    // Cabecera del CSV
    archivo << "Simulaciones,Mersenne_VaR95,Antithetic_VaR95,Sobol_VaR95\n";

    // 4. Parámetros del bucle de convergencia
    const int SIMS_INICIALES = 128;
    const int SIMS_MAXIMAS = std::pow(2, 20);
    const int PASO_SIMS = 2;

    std::cout << "Evaluando desde " << SIMS_INICIALES << " hasta " << SIMS_MAXIMAS << " simulaciones...\n";

    // 5. Ejecución iterativa aumentando N
    for (int sims = SIMS_INICIALES; sims <= SIMS_MAXIMAS; sims *= PASO_SIMS) {
        config.totalSims = sims;

        // --- A) Mersenne Twister (Pseudo-Random estándar) ---
        config.rngType = RNGType::MersenneTwister;
        RiskCalculator calcMT = MonteCarloEngine::run(cartera, config);
        double varMT = calcMT.calculateVaR(0.99);

        // --- B) Variables Antitéticas (Reducción de Varianza) ---
        config.rngType = RNGType::Antithetic;
        RiskCalculator calcAnti = MonteCarloEngine::run(cartera, config);
        double varAnti = calcAnti.calculateVaR(0.99);

        // --- C) Secuencia de Sobol (Quasi-Random) ---
        config.rngType = RNGType::Sobol;
        RiskCalculator calcSobol = MonteCarloEngine::run(cartera, config);
        double varSobol = calcSobol.calculateVaR(0.99);

        // Guardar métricas en el CSV
        archivo << sims << "," << varMT << "," << varAnti << "," << varSobol << "\n";
        
        // Feedback visual en consola
        std::cout << "N = " << sims 
                  << " | MT: " << varMT 
                  << " | Anti: " << varAnti 
                  << " | Sobol: " << varSobol << "\n";
    }
    
    archivo.close();
    std::cout << "--> Benchmark de convergencia finalizado. Datos guardados en CSV.\n";
    
    BOOST_CHECK(true); 
}

BOOST_AUTO_TEST_SUITE_END()

// ========================================================================
// NUEVA SUITE: EJEMPLO COMPLETO CON HULL-WHITE
// ========================================================================

BOOST_AUTO_TEST_SUITE(Escenario_HullWhite_Completo)

BOOST_AUTO_TEST_CASE(Ejemplo_Con_Tipos_y_Equity) {
    std::cout << "\n--> Iniciando ejemplo completo con Hull-White...\n";

    // 1) Curva inicial (zero rates) y parámetros Hull-White
    //    meanReversion=0.05, volTipos=0.01 — ajustar aquí para calibración futura
    YieldCurve curve({0.5, 1.0, 2.0, 5.0, 10.0}, {0.02, 0.022, 0.025, 0.03, 0.032});
    auto hw = std::make_shared<HullWhiteModel>(0.05, 0.01, curve);

    // 2) Cartera con equity + bono (bono usa el modelo de tipos)
    Portfolio cartera;
    auto stock = std::make_shared<GBMAsset>("EquityA", 100.0, 0.02, 0.20);
    auto bond = std::make_shared<CouponBond>("Bond_HW", 1000.0, 0.03, 3.0, hw);
    cartera.addPosition(stock, 50.0);
    cartera.addPosition(bond, 5.0);

    // 3) SimConfig: todos los campos explícitos (con dinámica de tipos activa)
    SimConfig config;
    config.totalSims = 20000;
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = static_cast<int>(std::thread::hardware_concurrency());
    //config.rngType = RNGType::MersenneTwister;
    config.rngType = RNGType::Antithetic;
    //config.rngType = RNGType::Sobol;
    config.rateModel = hw;             // Activa ratePath, drift acoplado a r_t y descuento
    config.computeStandardErrors = true;   // Ejemplo didáctico: reportar VaR/ES ± error estándar
    config.bootstrapReplications = 256;    // ES: bootstrap (esquema CLT con MT)
    config.sobolBatchCount = 32;           // Ignorado aquí (rngType != Sobol)

    // 4) Correlación (numActivos + 1) × (numActivos + 1): [Equity, Bond, Rate]
    Eigen::MatrixXd corr(3, 3);
    corr << 1.0, 0.2, 0.3,
            0.2, 1.0, 0.4,
            0.3, 0.4, 1.0;
    config.corrMatrix = corr;

    BOOST_CHECK_NO_THROW({
        RiskCalculator report = MonteCarloEngine::run(cartera, config);
        const RiskEstimate var99 = report.estimateVaR(0.99);
        const RiskEstimate es99 = report.estimateES(0.99);
        std::cout << "VaR 99% (HW): " << var99.value;
        if (var99.hasStandardError()) {
            std::cout << " +/- " << var99.standardError;
        }
        std::cout << "\n";
        std::cout << "ES  99% (HW): " << es99.value;
        if (es99.hasStandardError()) {
            std::cout << " +/- " << es99.standardError;
        }
        std::cout << "\n";
        BOOST_CHECK(std::isfinite(var99.value));
        BOOST_CHECK(std::isfinite(es99.value));
        BOOST_CHECK(report.standardErrorsEnabled());
        BOOST_CHECK(var99.hasStandardError());
        BOOST_CHECK(es99.hasStandardError());
    });
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// CARTERA BASE COMPLETA: todos los tipos de activo + Hull-White
// ============================================================================

BOOST_AUTO_TEST_SUITE(Escenario_Cartera_Base)

BOOST_AUTO_TEST_CASE(Cartera_Completa_MultiActivo_HullWhite) {
    std::cout << "\n--> Cartera base completa (GBM + Europea + Barrera + Bono HW)...\n";

    const CarteraBaseCompleta base = makeCarteraBaseCompleta();

    BOOST_CHECK_EQUAL(base.portfolio.getNumAssets(), 4u);
    BOOST_CHECK_GT(base.initialValue, 0.0);

    Eigen::LLT<Eigen::MatrixXd> llt(base.corrMatrix);
    BOOST_CHECK_MESSAGE(llt.info() == Eigen::Success,
                        "La matriz de correlación debe ser definida positiva");

    std::cout << "Valor inicial cartera: " << base.initialValue << "\n";
    std::cout << "Activos: EquityIndex | EuroCall_Tech | BarrierCall_Ind | GovBond_HW\n";
    std::cout << "Factores correlacionados: 4 activos + 1 factor tasa (Hull-White)\n";

    SimConfig config;
    config.totalSims = 20000;
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = static_cast<int>(std::thread::hardware_concurrency());
    config.rngType = RNGType::MersenneTwister;
    config.rateModel = base.hw;
    config.computeStandardErrors = true;
    config.bootstrapReplications = 256;
    config.sobolBatchCount = 32;
    config.corrMatrix = base.corrMatrix;

    BOOST_CHECK_NO_THROW({
        RiskCalculator report = MonteCarloEngine::run(base.portfolio, config);

        const RiskEstimate meanPnL = report.estimateMeanPnL();
        const RiskEstimate var99 = report.estimateVaR(0.99);
        const RiskEstimate var95 = report.estimateVaR(0.95);
        const RiskEstimate es99 = report.estimateES(0.99);
        const RiskEstimate es95 = report.estimateES(0.95);

        std::cout << "Media PnL (1Y): " << meanPnL.value;
        if (meanPnL.hasStandardError()) {
            std::cout << " +/- " << meanPnL.standardError;
        }
        std::cout << "\n";

        std::cout << "VaR 99%: " << var99.value;
        if (var99.hasStandardError()) {
            std::cout << " +/- " << var99.standardError;
        }
        std::cout << " | VaR 95%: " << var95.value << "\n";

        std::cout << "ES  99%: " << es99.value;
        if (es99.hasStandardError()) {
            std::cout << " +/- " << es99.standardError;
        }
        std::cout << " | ES  95%: " << es95.value << "\n";

        BOOST_CHECK(std::isfinite(meanPnL.value));
        BOOST_CHECK(std::isfinite(var99.value));
        BOOST_CHECK(std::isfinite(var95.value));
        BOOST_CHECK(std::isfinite(es99.value));
        BOOST_CHECK(std::isfinite(es95.value));

        BOOST_CHECK_LE(es99.value, var99.value);
        BOOST_CHECK_LE(es95.value, var95.value);

        BOOST_CHECK(report.standardErrorsEnabled());
        BOOST_CHECK(var99.hasStandardError());
        BOOST_CHECK(es99.hasStandardError());
    });
}

BOOST_AUTO_TEST_CASE(Convergencia_Generadores_Con_Errores) {
    std::cout << "\n--> Convergencia RNG (cartera base + Hull-White) con errores MC...\n";

    const CarteraBaseCompleta base = makeCarteraBaseCompleta();

    SimConfig config;
    config.totalSims = 1024;  // Sobrescrito en el bucle
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = static_cast<int>(std::thread::hardware_concurrency());
    config.rateModel = base.hw;
    config.corrMatrix = base.corrMatrix;
    config.computeStandardErrors = true;
    config.bootstrapReplications = 256;
    config.sobolBatchCount = 32;
    config.rngSeed = 1234;  // Semilla alternativa para regenerar escenarios MC

    const std::string rutaCSV = "../graphics/convergencia/convergencia_cartera_base.csv";
    std::ofstream archivo(rutaCSV);
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/convergencia/convergencia_cartera_base.csv");
    }

    archivo << std::setprecision(12);
    archivo << "N,RNG,MeanPnL,MeanPnL_SE,VaR95,VaR95_SE,ES95,ES95_SE,VaR99,VaR99_SE,ES975,ES975_SE\n";

    const int simsIniciales = 1024/4;
    const int simsMaximas = 32768*4;
    const int pasoSims = 2;

    std::cout << "Cartera base (V0=" << base.initialValue << "), N desde "
              << simsIniciales << " hasta " << simsMaximas << " (×" << pasoSims
              << "), rngSeed=" << config.rngSeed << ")\n";

    for (int sims = simsIniciales; sims <= simsMaximas; sims *= pasoSims) {
        config.totalSims = sims;

        config.rngType = RNGType::MersenneTwister;
        const RiskCalculator calcMT = MonteCarloEngine::run(base.portfolio, config);
        appendConvergenceRow(archivo, sims, "MersenneTwister", calcMT);

        config.rngType = RNGType::Antithetic;
        const RiskCalculator calcAnti = MonteCarloEngine::run(base.portfolio, config);
        appendConvergenceRow(archivo, sims, "Antithetic", calcAnti);

        config.rngType = RNGType::Sobol;
        const RiskCalculator calcSobol = MonteCarloEngine::run(base.portfolio, config);
        appendConvergenceRow(archivo, sims, "Sobol", calcSobol);

        const RiskEstimate var99MT = calcMT.estimateVaR(0.99);
        const RiskEstimate var99Anti = calcAnti.estimateVaR(0.99);
        const RiskEstimate var99Sobol = calcSobol.estimateVaR(0.99);

        std::cout << "N=" << sims
                  << " | MT VaR99=" << var99MT.value;
        if (var99MT.hasStandardError()) {
            std::cout << "+/-" << var99MT.standardError;
        }
        std::cout << " | Anti=" << var99Anti.value;
        if (var99Anti.hasStandardError()) {
            std::cout << "+/-" << var99Anti.standardError;
        }
        std::cout << " | Sobol=" << var99Sobol.value;
        if (var99Sobol.hasStandardError()) {
            std::cout << "+/-" << var99Sobol.standardError;
        }
        std::cout << '\n';

        BOOST_CHECK(std::isfinite(var99MT.value));
        BOOST_CHECK(std::isfinite(var99Anti.value));
        BOOST_CHECK(std::isfinite(var99Sobol.value));
    }

    archivo.close();
    std::cout << "--> Datos guardados en " << rutaCSV << '\n';
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()


// ============================================================================
// BENCHMARK LAYOUT: speedup RowMajor vs ColMajor en función del nº de activos.
// Justificación dimensional: Z ∈ R^{F×S} (factores × pasos temporales); el motor
// consume filas completas por activo (simulatePathPnL). Con S=50 fijo (producción)
// y F creciente vía réplicas de benchmark1, RowMajor mantiene contigüidad en el
// patrón de acceso dominante (filas largas en F, cortas en S). ColMajor usa el
// mismo pipeline cambiando solo el StorageOrder de Eigen (sin copia intermedia).
// ============================================================================

BOOST_AUTO_TEST_SUITE(Escenario_Benchmark_Layout)

BOOST_AUTO_TEST_CASE(Layout_Speedup_vs_NumActivos) {
    std::cout << "\n--> Benchmark layout: speedup RowMajor/ColMajor vs. nº activos...\n";
    std::cout << "    Geometría Z: (factores × pasos); RowMajor → filas contiguas por activo.\n";

    const std::vector<int> replicaCounts = {
        16, 32, 64, 128, 256, 512, 1024, 2048,
    };

    SimConfig config;
    config.totalTime = 1.0;
    config.numSteps = 50;  // Horizonte temporal fijo; crece solo el nº de factores F
    config.numCores = layoutBenchmarkNumCores();
    config.rngType = RNGType::MersenneTwister;
    config.rngSeed = 1235;
    config.computeStandardErrors = false;

    const std::string rutaCSV = "../graphics/layout_benchmark/benchmark_layout_speedup_activos.csv";
    std::ofstream archivo(rutaCSV);
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/layout_benchmark/benchmark_layout_speedup_activos.csv");
    }

    archivo << std::setprecision(12);
    archivo << "NumReplicas,NumAssets,NumSteps,N,PathDim,TimeRowMs,TimeColMs,SpeedupColOverRow,VaR99Row,VaR99Col\n";

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "numCores=" << config.numCores << ", T=" << config.totalTime
              << " año, pasos=" << config.numSteps << " (F×S = factores × pasos)\n\n";

    for (int replicas : replicaCounts) {
        std::cout << ">>> Construyendo cartera (" << replicas << " réplicas)...\n";
        std::cout.flush();

        const CarteraBaseCompleta bench = makeCarteraBenchmarkReplicada(replicas);
        const int numAssets = static_cast<int>(bench.portfolio.getNumAssets());
        const int pathDim = (numAssets + 1) * config.numSteps;
        const int sims = simsForLayoutBenchmark(numAssets);
        const int repetitions = layoutBenchmarkRepetitions(numAssets);

        config.totalSims = sims;
        config.rateModel = bench.hw;
        config.corrMatrix = bench.corrMatrix;

        std::cout << ">>> Factorizando correlación " << bench.corrMatrix.rows() << "×"
                  << bench.corrMatrix.cols() << "...\n";
        std::cout.flush();

        Eigen::LLT<Eigen::MatrixXd> llt(bench.corrMatrix);
        BOOST_REQUIRE_MESSAGE(llt.info() == Eigen::Success,
                              "Matriz de correlación no definida positiva (réplicas=" << replicas << ")");

        std::cout << ">>> Midiendo (" << repetitions << " repeticiones, N=" << sims << ")...\n";
        std::cout.flush();

        const LayoutBenchmarkPair measured =
            runLayoutBenchmarkMedianPair(bench.portfolio, config, repetitions);
        const LayoutBenchmarkResult& row = measured.row;
        const LayoutBenchmarkResult& col = measured.col;
        const double speedup = measured.speedup;

        std::cout << "réplicas=" << replicas
                  << " | activos=" << numAssets
                  << " | N=" << sims
                  << " | dim=" << pathDim
                  << " | Row=" << row.elapsedMs << " ms"
                  << " | Col=" << col.elapsedMs << " ms"
                  << " | speedup=" << speedup << "x\n";

        archivo << replicas << ',' << numAssets << ',' << config.numSteps << ','
                << sims << ',' << pathDim << ','
                << row.elapsedMs << ',' << col.elapsedMs << ',' << speedup << ','
                << row.var99 << ',' << col.var99 << '\n';
        archivo.flush();

        BOOST_CHECK(std::isfinite(row.var99));
        BOOST_CHECK(std::isfinite(col.var99));
        BOOST_CHECK_CLOSE(row.var99, col.var99, 0.01);
        BOOST_CHECK_GT(row.elapsedMs, 0.0);
        BOOST_CHECK_GT(col.elapsedMs, 0.0);
        BOOST_CHECK_GT(speedup, 0.0);
    }

    archivo.close();
    std::cout << "\n--> Datos guardados en " << rutaCSV << '\n';
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(Rendimiento_Eigen)

BOOST_AUTO_TEST_CASE(Rendimiento) {
    // Benchmark de localidad de caché en Eigen (independiente de SimConfig / MonteCarloEngine)
    const int numAssets = 100;
    const int steps = 1000;
    const int iterations = 2000;

    // Matriz de Eigen (Por defecto: Column-Major)
    Eigen::MatrixXd Z_corr = Eigen::MatrixXd::Random(numAssets, steps);
    
    // El destino clásico (Vector de vectores)
    std::vector<std::vector<double>> Z_path(numAssets, std::vector<double>(steps, 0.0));

    // ==========================================
    // TEST 1: RECORRIDO INCORRECTO (Mal acceso a Caché)
    // Activos fuera (filas), Pasos dentro (columnas)
    // ==========================================
    auto start_bad = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        for (int a = 0; a < numAssets; ++a) {
            for (int s = 0; s < steps; ++s) {
                // Saltando de columna en columna en memoria contigua -> Cache Miss
                Z_path[a][s] = Z_corr(a, s); 
            }
        }
    }
    
    auto end_bad = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_bad = end_bad - start_bad;

    // Aseguramos que el compilador no haya eliminado el bucle
    BOOST_CHECK_NE(Z_path[0][0], 999.0); 

    // ==========================================
    // TEST 2: RECORRIDO OPTIMIZADO (Buen acceso a Caché)
    // Pasos fuera (columnas), Activos dentro (filas)
    // ==========================================
    auto start_good = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        for (int s = 0; s < steps; ++s) {
            for (int a = 0; a < numAssets; ++a) {
                // Leyendo la columna de Eigen verticalmente hacia abajo -> Memoria contigua
                Z_path[a][s] = Z_corr(a, s); 
            }
        }
    }
    
    auto end_good = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_good = end_good - start_good;

    // Aseguramos que el compilador no haya eliminado el bucle
    BOOST_CHECK_EQUAL(Z_path[numAssets - 1][steps - 1], Z_corr(numAssets - 1, steps - 1));

    // ==========================================
    // RESULTADOS
    // ==========================================
    BOOST_TEST_MESSAGE("\n==========================================");
    BOOST_TEST_MESSAGE("  BENCHMARK: LOCALIDAD DE CACHÉ EN EIGEN  ");
    BOOST_TEST_MESSAGE("==========================================");
    BOOST_TEST_MESSAGE("Tiempo Recorrido Incorrecto: " << elapsed_bad.count() << " ms");
    BOOST_TEST_MESSAGE("Tiempo Recorrido Optimizado: " << elapsed_good.count() << " ms");
    
    double mejora = ((elapsed_bad.count() - elapsed_good.count()) / elapsed_bad.count()) * 100.0;
    BOOST_TEST_MESSAGE("Rendimiento ganado: " << mejora << "%");
    BOOST_TEST_MESSAGE("==========================================\n");

    // El test pasará si la versión optimizada es estrictamente más rápida
    BOOST_CHECK_LT(elapsed_good.count(), elapsed_bad.count());
}

BOOST_AUTO_TEST_SUITE_END()