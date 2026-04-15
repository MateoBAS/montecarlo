#include "RiskCalculator.h"
#include <algorithm>
#include <stdexcept>

RiskCalculator::RiskCalculator(const std::vector<double>& simulatedPnLs) {
    sortedPnLs = simulatedPnLs;
    std::sort(sortedPnLs.begin(), sortedPnLs.end());
}

double RiskCalculator::calculateVaR(double confidenceLevel) const {
    if (confidenceLevel <= 0.0 || confidenceLevel >= 1.0) {
        throw std::invalid_argument("El nivel de confianza debe estar entre 0 y 1 (ej. 0.99)");
    }

    double alpha = 1.0 - confidenceLevel;
    
    size_t index = static_cast<size_t>(sortedPnLs.size() * alpha);

    return sortedPnLs[index];
}

double RiskCalculator::calculateES(double confidenceLevel) const {
    if (confidenceLevel <= 0.0 || confidenceLevel >= 1.0) {
        throw std::invalid_argument("El nivel de confianza debe estar entre 0 y 1 (ej. 0.99)");
    }

    double alpha = 1.0 - confidenceLevel;
    size_t index = static_cast<size_t>(sortedPnLs.size() * alpha);

    if (index == 0) return sortedPnLs[0];

    double sumOfTailLosses = 0.0;
    for (size_t i = 0; i < index; ++i) {
        sumOfTailLosses += sortedPnLs[i];
    }

    return sumOfTailLosses / index;
}