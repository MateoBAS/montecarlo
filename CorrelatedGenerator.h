#ifndef CORRELATED_GENERATOR_H
#define CORRELATED_GENERATOR_H

#include <vector>
#include <Eigen/Dense>
#include "RandomGenerator.h"

class CorrelatedGenerator {
private:
    RandomGenerator* rng; // Puntero a nuestro generador base
    Eigen::MatrixXd L;    // Matriz triangular inferior de Cholesky
    int numAssets;

public:
    // Constructor: recibe el generador y la matriz de correlación
    CorrelatedGenerator(RandomGenerator* rng, const Eigen::MatrixXd& correlationMatrix);
    
    // Genera y devuelve un vector de Zs correlacionadas
    std::vector<double> getCorrelatedNormals();
};

#endif // CORRELATED_GENERATOR_H