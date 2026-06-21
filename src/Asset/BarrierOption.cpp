#include "BarrierOption.h"
#include <cmath>
#include <stdexcept>

BarrierOption::BarrierOption(std::string name, double optionPremium, double underlyingInitialPrice,
                             double strike, double barrier, double drift, double volatility)
    : Asset(std::move(name), optionPremium), underlyingInitialPrice(underlyingInitialPrice),
      strike(strike), barrier(barrier), driftSpread(drift), volatility(volatility) {}

double BarrierOption::simulateBarrierPayoff(double totalTime, int numSteps,
                                            const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                            const std::vector<double>& ratePath) const {
    if (!ratePath.empty() && static_cast<int>(ratePath.size()) != numSteps + 1) {
        throw std::invalid_argument("Rate path size must be numSteps + 1.");
    }
    if (z_shocks.size() != numSteps) {
        throw std::invalid_argument("z_shocks size must match numSteps.");
    }

    if (numSteps <= 0) {
        return std::max(underlyingInitialPrice - strike, 0.0);
    }

    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double vol_term = volatility * std::sqrt(dt);
    double currentPrice = underlyingInitialPrice;
    bool knockedOut = false;

    for (int i = 0; i < numSteps; ++i) {
        double rate = ratePath.empty() ? 0.0 : ratePath[i];
        double drift_term = (rate + driftSpread - variance_penalty) * dt;
        double exponent = drift_term + vol_term * z_shocks[i];
        currentPrice *= std::exp(exponent);

        if (currentPrice >= barrier) {
            knockedOut = true;
        }
    }

    if (knockedOut) {
        return 0.0;
    }
    return std::max(currentPrice - strike, 0.0);
}

double BarrierOption::simulateFinalValue(double totalTime, int numSteps,
                                         const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                         const std::vector<double>& ratePath) const {
    return simulateBarrierPayoff(totalTime, numSteps, z_shocks, ratePath);
}

std::vector<double> BarrierOption::generatePath(double totalTime, int numSteps,
                                                const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                                const std::vector<double>& ratePath) const {
    return {getInitialPrice(), simulateFinalValue(totalTime, numSteps, z_shocks, ratePath)};
}
