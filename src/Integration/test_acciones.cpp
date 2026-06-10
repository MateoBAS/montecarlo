#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <memory>
#include <numeric>
#include <cmath>
#include <thread>
#include <Eigen/Dense>

#include "MontecarloEngine/MontecarloEngine.h"
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"
#include "Asset/GBMAsset.h"
#include "Asset/EuropeanOption.h"
#include "Asset/CouponBond.h"
#include "InterestRate/YieldCurve.h"
#include "InterestRate/HullWhiteModel.h"

BOOST_AUTO_TEST_SUITE(Escenario_Rendimiento_Estadistico)

BOOST_AUTO_TEST_CASE(Amdahl_Con_Incertidumbre) {
    std::cout << "--> Iniciando Benchmark Estadistico de Amdahl...\n";
    
    Portfolio cartera;
    auto techA = std::make_shared<GBMAsset>("TechA", 150.0, 0.05, 0.25);
    auto techB = std::make_shared<GBMAsset>("TechB", 200.0, 0.08, 0.30);
    cartera.addPosition(techA, 50.0);
    cartera.addPosition(techB, 30.0);

    SimConfig config;
    config.totalSims = 100000; 
    config.totalTime = 10.0 / 252.0;
    config.numSteps = 10;
    
    Eigen::MatrixXd corr(2, 2);
    corr << 1.0, 0.6, 0.6, 1.0;
    config.corrMatrix = corr;

    // PARÁMETROS ESTADÍSTICOS
    const int maxHilos = std::thread::hardware_concurrency();
    const int medidasPorPunto = 1000;
    const int iteracionesCalentamiento = 25;
    
    std::string rutaCSV = "../graphics/amdahl_estadistico_curvas_tipos.csv";
    std::ofstream archivo(rutaCSV);
    
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/");
    }
    
    // Nueva cabecera con datos estadísticos
    archivo << "Hilos,Tiempo_Medio_ms,Desviacion_ms\n";
    
    std::cout << "Mediciones por punto: " << medidasPorPunto << "\n";
    std::cout << "Por favor, no uses el PC mientras se ejecuta este test...\n\n";

    for (int hilos = 1; hilos <= maxHilos; ++hilos) {
        config.numCores = hilos;
        
        // 1. FASE DE CALENTAMIENTO (Warm-up)
        // Despierta los hilos del SO y llena la caché L1/L2/L3
        for(int w = 0; w < iteracionesCalentamiento; ++w) {
            MonteCarloEngine::run(cartera, config);
        }

        // 2. FASE DE MEDICIÓN
        std::vector<double> tiempos(medidasPorPunto);
        for (int m = 0; m < medidasPorPunto; ++m) {
            auto inicio = std::chrono::high_resolution_clock::now();
            
            RiskCalculator report = MonteCarloEngine::run(cartera, config);
            volatile double var = report.calculateVaR(0.99); // Evita que el compilador ignore el código
            
            auto fin = std::chrono::high_resolution_clock::now();
            tiempos[m] = std::chrono::duration<double, std::milli>(fin - inicio).count();
        }
        
        // 3. CÁLCULO ESTADÍSTICO (Media y Desviación Estándar)
        double sumaTiempos = std::accumulate(tiempos.begin(), tiempos.end(), 0.0);
        double media = sumaTiempos / medidasPorPunto;
        
        double sumaDiferenciasCuadrado = 0.0;
        for (double t : tiempos) {
            sumaDiferenciasCuadrado += (t - media) * (t - media);
        }
        double varianza = sumaDiferenciasCuadrado / (medidasPorPunto - 1); // Varianza muestral
        double desviacionEstandar = std::sqrt(varianza);

        archivo << hilos << "," << media << "," << desviacionEstandar << "\n";
        
        std::cout << "Hilos: " << hilos 
                  << " | T_Medio: " << media << " ms"
                  << " | StdDev: ±" << desviacionEstandar << " ms\n";
                  
        // 4. PREVENCIÓN DE ESTRANGULAMIENTO TÉRMICO (Cool-down)
        // Damos 500ms al procesador para que disipe el calor antes de la siguiente tanda
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    archivo.close();
    BOOST_CHECK(true); 
}

BOOST_AUTO_TEST_CASE(Amdahl_Con_Incertidumbre_Cartera_Grande) {
    std::cout << "--> Iniciando benchmark de Amdahl con cartera grande...\n";

    Portfolio cartera;
    std::vector<std::shared_ptr<GBMAsset>> activos;
    activos.reserve(16);

    activos.push_back(std::make_shared<GBMAsset>("TechA", 150.0, 0.05, 0.25));
    activos.push_back(std::make_shared<GBMAsset>("TechB", 200.0, 0.08, 0.30));
    activos.push_back(std::make_shared<GBMAsset>("BankA", 90.0, 0.04, 0.18));
    activos.push_back(std::make_shared<GBMAsset>("BankB", 110.0, 0.045, 0.20));
    activos.push_back(std::make_shared<GBMAsset>("EnergyA", 75.0, 0.03, 0.28));
    activos.push_back(std::make_shared<GBMAsset>("EnergyB", 85.0, 0.035, 0.26));
    activos.push_back(std::make_shared<GBMAsset>("RetailA", 60.0, 0.025, 0.22));
    activos.push_back(std::make_shared<GBMAsset>("RetailB", 72.0, 0.028, 0.24));
    activos.push_back(std::make_shared<GBMAsset>("IndustrialA", 130.0, 0.04, 0.19));
    activos.push_back(std::make_shared<GBMAsset>("IndustrialB", 140.0, 0.042, 0.21));
    activos.push_back(std::make_shared<GBMAsset>("PharmaA", 180.0, 0.055, 0.17));
    activos.push_back(std::make_shared<GBMAsset>("PharmaB", 165.0, 0.05, 0.16));

    for (size_t i = 0; i < activos.size(); ++i) {
        cartera.addPosition(activos[i], 20.0 + static_cast<double>(i) * 2.5);
    }

    SimConfig config;
    config.totalSims = 100000;
    config.totalTime = 10.0 / 252.0;
    config.numSteps = 10;

    Eigen::MatrixXd corr = Eigen::MatrixXd::Identity(static_cast<int>(activos.size()), static_cast<int>(activos.size()));
    for (int i = 0; i < corr.rows(); ++i) {
        for (int j = i + 1; j < corr.cols(); ++j) {
            double value = 0.15 + 0.01 * static_cast<double>((i + j) % 5);
            corr(i, j) = value;
            corr(j, i) = value;
        }
    }
    config.corrMatrix = corr;

    const int maxHilos = std::thread::hardware_concurrency();
    const int medidasPorPunto = 300;
    const int iteracionesCalentamiento = 10;

    std::string rutaCSV = "../graphics/amdahl_estadistico_cartera_grande.csv";
    std::ofstream archivo(rutaCSV);

    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV en ../graphics/");
    }

    archivo << "Hilos,Tiempo_Medio_ms,Desviacion_ms\n";

    std::cout << "Activos en cartera: " << activos.size() << "\n";
    std::cout << "Mediciones por punto: " << medidasPorPunto << "\n";
    std::cout << "Por favor, no uses el PC mientras se ejecuta este test...\n\n";

    for (int hilos = 1; hilos <= maxHilos; ++hilos) {
        config.numCores = hilos;

        for (int w = 0; w < iteracionesCalentamiento; ++w) {
            MonteCarloEngine::run(cartera, config);
        }

        std::vector<double> tiempos(medidasPorPunto);
        for (int m = 0; m < medidasPorPunto; ++m) {
            auto inicio = std::chrono::high_resolution_clock::now();

            RiskCalculator report = MonteCarloEngine::run(cartera, config);
            volatile double var = report.calculateVaR(0.99);

            auto fin = std::chrono::high_resolution_clock::now();
            tiempos[m] = std::chrono::duration<double, std::milli>(fin - inicio).count();
        }

        double sumaTiempos = std::accumulate(tiempos.begin(), tiempos.end(), 0.0);
        double media = sumaTiempos / medidasPorPunto;

        double sumaDiferenciasCuadrado = 0.0;
        for (double t : tiempos) {
            sumaDiferenciasCuadrado += (t - media) * (t - media);
        }
        double varianza = sumaDiferenciasCuadrado / (medidasPorPunto - 1);
        double desviacionEstandar = std::sqrt(varianza);

        archivo << hilos << "," << media << "," << desviacionEstandar << "\n";

        std::cout << "Hilos: " << hilos
                  << " | T_Medio: " << media << " ms"
                  << " | StdDev: ±" << desviacionEstandar << " ms\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    archivo.close();
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

    // 2. Configuración base del motor
    SimConfig config;
    config.totalTime = 1.0; // 1 año de proyección
    config.numSteps = 252;  // Simulación diaria
    config.numCores = std::thread::hardware_concurrency();
    
    Eigen::MatrixXd corr(2, 2);
    corr << 1.0, 0.4, 
            0.4, 1.0;
    config.corrMatrix = corr;

    // 3. Preparación del archivo de salida
    std::string rutaCSV = "../graphics/convergencia_rng.csv";
    std::ofstream archivo(rutaCSV);
    
    if (!archivo.is_open()) {
        BOOST_FAIL("Fallo al crear el archivo CSV para la convergencia en ../graphics/");
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
        double varMT = calcMT.calculateVaR(0.95);

        // --- B) Variables Antitéticas (Reducción de Varianza) ---
        config.rngType = RNGType::Antithetic;
        RiskCalculator calcAnti = MonteCarloEngine::run(cartera, config);
        double varAnti = calcAnti.calculateVaR(0.95);

        // --- C) Secuencia de Sobol (Quasi-Random) ---
        config.rngType = RNGType::Sobol;
        RiskCalculator calcSobol = MonteCarloEngine::run(cartera, config);
        double varSobol = calcSobol.calculateVaR(0.95);

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

    // 1) Curva inicial (zero rates) y modelo HW
    YieldCurve curve({0.5, 1.0, 2.0, 5.0, 10.0}, {0.02, 0.022, 0.025, 0.03, 0.032});
    auto hw = std::make_shared<HullWhiteModel>(0.05, 0.01, curve);

    // 2) Cartera con equity + bono (bono usa el modelo de tipos)
    Portfolio cartera;
    auto stock = std::make_shared<GBMAsset>("EquityA", 100.0, 0.02, 0.20);
    auto bond = std::make_shared<CouponBond>("Bond_HW", 1000.0, 0.03, 3.0, hw);
    cartera.addPosition(stock, 50.0);
    cartera.addPosition(bond, 5.0);

    // 3) Configuración del motor
    SimConfig config;
    config.totalSims = 2000;
    config.totalTime = 1.0;
    config.numSteps = 50;
    config.numCores = std::thread::hardware_concurrency();
    config.rateModel = hw;

    // 4) Matriz de correlación: [Equity, Bond, Rate]
    Eigen::MatrixXd corr(3, 3);
    corr << 1.0, 0.2, 0.3,
            0.2, 1.0, 0.4,
            0.3, 0.4, 1.0;
    config.corrMatrix = corr;

    BOOST_CHECK_NO_THROW({
        RiskCalculator report = MonteCarloEngine::run(cartera, config);
        double var99 = report.calculateVaR(0.99);
        double es99 = report.calculateES(0.99);
        std::cout << "VaR 99% (HW): " << var99 << "\n";
        std::cout << "ES  99% (HW): " << es99 << "\n";
        BOOST_CHECK(std::isfinite(var99));
        BOOST_CHECK(std::isfinite(es99));
    });
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(Rendimiento_Eigen)

BOOST_AUTO_TEST_CASE(Rendimiento) {
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