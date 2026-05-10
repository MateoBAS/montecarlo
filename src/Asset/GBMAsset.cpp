#include "GBMAsset.h"

GBMAsset::GBMAsset(std::string name, double initialPrice, double drift, double volatility)
    : Asset(std::move(name), initialPrice), drift(drift), volatility(volatility) {}

std::vector<double> GBMAsset::generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const {
    return simulateGbmPath(initialPrice, drift, volatility, totalTime, numSteps, Zs);
}