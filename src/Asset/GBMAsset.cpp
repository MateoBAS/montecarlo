#include "GBMAsset.h"
#include <cmath>

GBMAsset::GBMAsset(std::string name, double initialPrice, double drift, double volatility)
    : Asset(std::move(name), initialPrice), drift(drift), volatility(volatility) {}

std::vector<double> GBMAsset::generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const {
    std::vector<double> path;
    path.reserve(numSteps + 1); 
    
    path.push_back(initialPrice); 
    
    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double drift_term = (drift - variance_penalty) * dt;
    double vol_term = volatility * std::sqrt(dt);
    
    double currentPrice = initialPrice;
    
    for (int i = 0; i < numSteps; ++i) {
        double exponent = drift_term + vol_term * Zs[i];
        currentPrice *= std::exp(exponent);
        path.push_back(currentPrice);
    }
    
    return path;
}