#include "EuropeanOption.h"

EuropeanOption::EuropeanOption(std::string name, double optionPremium, double underlyingInitialPrice,
                               double strike, double drift, double volatility, OptionType type)
    : Asset(std::move(name), optionPremium), underlyingInitialPrice(underlyingInitialPrice),
      strike(strike), driftSpread(drift), volatility(volatility), type(type) {}

double EuropeanOption::payoffFromTerminalSpot(double terminalSpot) const {
    if (type == OptionType::Call) {
        return std::max(terminalSpot - strike, 0.0);
    }
    return std::max(strike - terminalSpot, 0.0);
}

double EuropeanOption::simulateFinalValue(double totalTime, int numSteps,
                                          const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                          const std::vector<double>& ratePath) const {
    double terminalSpot = simulateGbmTerminal(underlyingInitialPrice, driftSpread, volatility,
                                              totalTime, numSteps, z_shocks, ratePath);
    return payoffFromTerminalSpot(terminalSpot);
}

std::vector<double> EuropeanOption::generatePath(double totalTime, int numSteps,
                                                 const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                                 const std::vector<double>& ratePath) const {
    return {getInitialPrice(), simulateFinalValue(totalTime, numSteps, z_shocks, ratePath)};
}
