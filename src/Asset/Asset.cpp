#include "Asset.h"
#include <cmath>
#include <stdexcept>

double Asset::simulateGbmEvolution(double startPrice, double driftSpread, double volatility,
                                   double totalTime, int numSteps,
                                   const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                   const std::vector<double>& ratePath,
                                   std::vector<double>* pathOut) const {
    if (!ratePath.empty() && static_cast<int>(ratePath.size()) != numSteps + 1) {
        throw std::invalid_argument("Rate path size must be numSteps + 1.");
    }
    if (z_shocks.size() != numSteps) {
        throw std::invalid_argument("z_shocks size must match numSteps.");
    }

    if (pathOut) {
        pathOut->clear();
        pathOut->reserve(static_cast<size_t>(numSteps) + 1);
        pathOut->push_back(startPrice);
    }

    if (numSteps <= 0) {
        return startPrice;
    }

    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double vol_term = volatility * std::sqrt(dt);
    double currentPrice = startPrice;

    for (int i = 0; i < numSteps; ++i) {
        double rate = ratePath.empty() ? 0.0 : ratePath[i];
        double drift_term = (rate + driftSpread - variance_penalty) * dt;
        double exponent = drift_term + vol_term * z_shocks[i];
        currentPrice *= std::exp(exponent);
        if (pathOut) {
            pathOut->push_back(currentPrice);
        }
    }

    return currentPrice;
}

double Asset::simulateGbmTerminal(double startPrice, double driftSpread, double volatility,
                                  double totalTime, int numSteps,
                                  const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                  const std::vector<double>& ratePath) const {
    return simulateGbmEvolution(startPrice, driftSpread, volatility, totalTime, numSteps,
                                z_shocks, ratePath, nullptr);
}

std::vector<double> Asset::simulateGbmPath(double startPrice, double drift, double volatility,
                                           double totalTime, int numSteps,
                                           const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks) const {
    std::vector<double> path;
    simulateGbmEvolution(startPrice, drift, volatility, totalTime, numSteps, z_shocks, {}, &path);
    return path;
}

std::vector<double> Asset::simulateGbmPathWithRate(double startPrice, double driftSpread, double volatility,
                                                   double totalTime, int numSteps,
                                                   const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                                   const std::vector<double>& ratePath) const {
    std::vector<double> path;
    simulateGbmEvolution(startPrice, driftSpread, volatility, totalTime, numSteps,
                         z_shocks, ratePath, &path);
    return path;
}
