#include "Asset.h"
#include <cmath>
#include <stdexcept>

std::vector<double> Asset::simulateGbmPath(double startPrice, double drift, double volatility,
                                           double totalTime, int numSteps,
                                           const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks) const {
    std::vector<double> path;
    path.reserve(numSteps + 1);

    path.push_back(startPrice);

    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double drift_term = (drift - variance_penalty) * dt;
    double vol_term = volatility * std::sqrt(dt);

    double currentPrice = startPrice;

    for (int i = 0; i < numSteps; ++i) {
        double exponent = drift_term + vol_term * z_shocks[i];
        currentPrice *= std::exp(exponent);
        path.push_back(currentPrice);
    }

    return path;
}

std::vector<double> Asset::simulateGbmPathWithRate(double startPrice, double driftSpread, double volatility,
                                                   double totalTime, int numSteps,
                                                   const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                                   const std::vector<double>& ratePath) const {
    if (!ratePath.empty() && static_cast<int>(ratePath.size()) != numSteps + 1) {
        throw std::invalid_argument("Rate path size must be numSteps + 1.");
    }

    std::vector<double> path;
    path.reserve(numSteps + 1);

    path.push_back(startPrice);

    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double vol_term = volatility * std::sqrt(dt);

    double currentPrice = startPrice;

    for (int i = 0; i < numSteps; ++i) {
        double rate = ratePath.empty() ? 0.0 : ratePath[i];
        double drift_term = (rate + driftSpread - variance_penalty) * dt;
        double exponent = drift_term + vol_term * z_shocks[i];
        currentPrice *= std::exp(exponent);
        path.push_back(currentPrice);
    }

    return path;
}
