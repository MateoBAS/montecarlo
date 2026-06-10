#include "GBMAsset.h"

GBMAsset::GBMAsset(std::string name, double initialPrice, double drift, double volatility)
    : Asset(std::move(name), initialPrice), driftSpread(drift), volatility(volatility) {}

std::vector<double> GBMAsset::generatePath(double totalTime, int numSteps,
                                           const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                           const std::vector<double>& ratePath) const {
    return simulateGbmPathWithRate(initialPrice, driftSpread, volatility, totalTime, numSteps, z_shocks, ratePath);
}