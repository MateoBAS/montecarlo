#include "CorrelatedGenerator.h"
#include <stdexcept>

CorrelatedGenerator::CorrelatedGenerator(RandomGenerator* rng, const Eigen::MatrixXd& correlationMatrix) 
    : rng(rng), numAssets(correlationMatrix.rows()) {
    
    if (correlationMatrix.rows() != correlationMatrix.cols()) {
        throw std::invalid_argument("La matriz de correlacion debe ser cuadrada.");
    }

    Eigen::LLT<Eigen::MatrixXd> lltOfCorr(correlationMatrix);
    if (lltOfCorr.info() == Eigen::NumericalIssue) {
        throw std::runtime_error("La matriz de correlacion no es definida positiva.");
    }
    
    L = lltOfCorr.matrixL();
}

std::vector<double> CorrelatedGenerator::getCorrelatedNormals() {
    Eigen::VectorXd independentZ(numAssets);
    for (int i = 0; i < numAssets; ++i) {
        independentZ(i) = rng->getStandardNormal();
    }

    Eigen::VectorXd correlatedZ = L * independentZ;

    std::vector<double> result(numAssets);
    for (int i = 0; i < numAssets; ++i) {
        result[i] = correlatedZ(i);
    }
    
    return result;
}