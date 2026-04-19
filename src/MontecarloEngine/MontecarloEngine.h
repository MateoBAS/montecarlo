#ifndef MONTE_CARLO_ENGINE_H
#define MONTE_CARLO_ENGINE_H

#include <vector>
#include <Eigen/Dense>
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"

struct SimConfig {
    int totalSims;
    double totalTime;
    int numSteps;
    Eigen::MatrixXd corrMatrix;
    int numCores;
};

class MonteCarloEngine {
public:
    // Este método centraliza toda la lógica que tenías en el main
    static RiskCalculator run(const Portfolio& portfolio, const SimConfig& config);

private:
    // Función auxiliar para los lotes paralelos
    static std::vector<double> runBatch(int sims, double time, int steps, 
                                        const Portfolio& port, const Eigen::MatrixXd& L, int seed);
};

#endif