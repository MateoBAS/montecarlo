#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <Eigen/Dense>

// Headers de nuestras clases
#include "GBMAsset.h"
#include "EuropeanOption.h"
#include "BarrierOption.h"
#include "CouponBond.h"
#include "Portfolio.h"
#include "RiskCalculator.h"

// Función para ejecutar las simulaciones en hilos paralelos
std::vector<double> runStandardBatch(int numSims, double totalTime, int numSteps, 
                                     const Portfolio& portfolio, const Eigen::MatrixXd& corrMatrix, 
                                     int seed) {
    
    size_t numAssets = portfolio.getNumAssets();
    Eigen::MatrixXd L = corrMatrix.llt().matrixL();
    
    std::mt19937 gen(seed);
    std::normal_distribution<double> normal_dist(0.0, 1.0);
    
    std::vector<double> results(numSims);
    
    for (int i = 0; i < numSims; ++i) {
        std::vector<std::vector<double>> Z_matrix(numAssets, std::vector<double>(numSteps));
        
        for (int s = 0; s < numSteps; ++s) {
            Eigen::VectorXd Z_step_indep(numAssets);
            for (size_t a = 0; a < numAssets; ++a) {
                Z_step_indep(a) = normal_dist(gen);
            }
            
            Eigen::VectorXd Z_step_corr = L * Z_step_indep;
            
            for (size_t a = 0; a < numAssets; ++a) {
                Z_matrix[a][s] = Z_step_corr(a);
            }
        }
        results[i] = portfolio.simulatePathPnL(totalTime, numSteps, Z_matrix);
    }
    return results;
}

int main() {
    std::cout << "--- Motor Quant: Cartera de Inversion Realista ---" << std::endl;

    // Parámetros de la simulación: 10 días de horizonte (VaR 10-d)
    double totalTime = 10.0 / 252.0; 
    int numSteps = 10;               

    // 1. ACCIONES (GBM)
    auto techA = std::make_shared<GBMAsset>("Apple_Style", 180.0, 0.05, 0.25);
    auto techB = std::make_shared<GBMAsset>("Nvidia_Style", 450.0, 0.08, 0.40);

    // 2. OPCIÓN EUROPEA (Call estándar sobre un tercer activo)
    // Prima: 12.0 | Subyacente: 200.0 | Strike: 210.0 | Drift: 0.05 | Vol: 0.30
    auto callEuro = std::make_shared<EuropeanOption>(
        "Call_Amazon_Style", 12.0, 200.0, 210.0, 0.05, 0.30, OptionType::Call
    );

    // 3. OPCIÓN BARRERA (Up-and-Out Call)
    // Si el precio toca 130, la opción muere.
    // Prima: 4.5 | Subyacente: 100.0 | Strike: 105.0 | Barrera: 130.0 | Drift: 0.05 | Vol: 0.25
    auto barrierOpt = std::make_shared<BarrierOption>(
        "Barrier_Call_Tesla_Style", 4.5, 100.0, 105.0, 130.0, 0.05, 0.25
    );

    // 4. RENTA FIJA (Bono del Estado con Cupones)
    // Nominal: 1000 | Cupón: 3.5% | Vencimiento: 5 años | TIR actual: 4% | Volatilidad TIR: 1.5% (0.015)
    auto bonoEstado = std::make_shared<CouponBond>(
        "Bono_Estado_5Y", 1000.0, 0.035, 5.0, 0.04, 0.015
    );

    // --- CONFIGURACIÓN DE LA CARTERA ---
    Portfolio miCartera;
    miCartera.addPosition(techA, 50.0);      // 50 acciones (~9.000€)
    miCartera.addPosition(techB, 20.0);      // 20 acciones (~9.000€)
    miCartera.addPosition(callEuro, 100.0);  // 100 contratos (~1.200€)
    miCartera.addPosition(barrierOpt, 50.0); // 50 contratos (~225€)
    miCartera.addPosition(bonoEstado, 20.0); // 20 bonos (~19.500€ aprox)

    std::cout << "Activos en cartera: " << miCartera.getNumAssets() << std::endl;

    // --- MATRIZ DE CORRELACIÓN (5x5) ---
    // [TechA, TechB, CallEuro, BarrierOpt, Bono]
    Eigen::MatrixXd corrMatrix(5, 5);
    corrMatrix << 1.0,  0.7,  0.5,  0.4, -0.2, // Tech A
                  0.7,  1.0,  0.6,  0.3, -0.2, // Tech B
                  0.5,  0.6,  1.0,  0.2, -0.1, // Call Euro
                  0.4,  0.3,  0.2,  1.0, -0.1, // Barrier
                 -0.2, -0.2, -0.1, -0.1,  1.0; // Bono (Correlación negativa = Diversificación)

    // --- MOTOR DE SIMULACIÓN ---
    int totalSimulaciones = 100000;
    int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4;
    int simsPerCore = totalSimulaciones / numCores;
    
    std::cout << "Lanzando " << totalSimulaciones << " simulaciones en " << numCores << " hilos...\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<std::vector<double>>> futures;
    for (int i = 0; i < numCores; ++i) {
        futures.push_back(
            std::async(std::launch::async, runStandardBatch, simsPerCore, totalTime, numSteps, 
                       std::ref(miCartera), std::ref(corrMatrix), 1234 + i)
        );
    }

    std::vector<double> pnlGlobal;
    pnlGlobal.reserve(totalSimulaciones);
    for (auto& fut : futures) {
        std::vector<double> batchResults = fut.get();
        pnlGlobal.insert(pnlGlobal.end(), batchResults.begin(), batchResults.end());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - startTime;

    std::cout << "Simulacion completada en " << elapsed.count() << " ms.\n" << std::endl;

    // --- CÁLCULO DE RESULTADOS ---
    RiskCalculator riskCalc(pnlGlobal);
    std::cout << "========== INFORME DE RIESGOS (10 DIAS) ==========" << std::endl;
    std::cout << "VaR (95%): " << riskCalc.calculateVaR(0.95) << " EUR" << std::endl;
    std::cout << "VaR (99%): " << riskCalc.calculateVaR(0.99) << " EUR" << std::endl;
    std::cout << "ES  (99%): " << riskCalc.calculateES(0.99) << " EUR" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}