#ifndef MONTE_CARLO_ENGINE_H
#define MONTE_CARLO_ENGINE_H

#include <vector>
#include <Eigen/Dense>
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"

// 1. Añadimos un enum para seleccionar el generador desde la configuración
enum class RNGType {
    MersenneTwister,
    Antithetic,
    Sobol
};

struct SimConfig {
    int totalSims;
    double totalTime;
    int numSteps;
    Eigen::MatrixXd corrMatrix;
    int numCores;
    RNGType rngType = RNGType::MersenneTwister;
};

class MonteCarloEngine {
public:
    static RiskCalculator run(const Portfolio& portfolio, const SimConfig& config);

private:
    // Cambiamos Eigen::MatrixXd& L por corrMatrix para poder usar tu CorrelatedGenerator
    static std::vector<double> runBatch(int sims, double time, int steps, 
                                        const Portfolio& port, const Eigen::MatrixXd& corrMatrix, 
                                        int seed, RNGType rngType);
};

#endif