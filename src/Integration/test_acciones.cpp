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
    
    std::string rutaCSV = "../graphics/amdahl_estadistico.csv";
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

BOOST_AUTO_TEST_SUITE_END()