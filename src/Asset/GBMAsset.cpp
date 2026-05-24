#include "GBMAsset.h"

GBMAsset::GBMAsset(std::string name, double initialPrice, double drift, double volatility)
    : Asset(std::move(name), initialPrice), driftSpread(drift), volatility(volatility) {}

std::vector<double> GBMAsset::generatePath(double totalTime, int numSteps,
                                           const std::vector<double>& Zs,
                                           const std::vector<double>& ratePath) const {
    return simulateGbmPathWithRate(initialPrice, driftSpread, volatility, totalTime, numSteps, Zs, ratePath);
}