#pragma once
#include "Asset.h"
#include <algorithm>

enum class OptionType { Call, Put };

class EuropeanOption : public Asset {
private:
    double underlyingInitialPrice;
    double strike;
    double driftSpread;
    double volatility;
    OptionType type;

    double payoffFromTerminalSpot(double terminalSpot) const;

public:
    EuropeanOption(std::string name, double optionPremium, double underlyingInitialPrice,
                   double strike, double drift, double volatility, OptionType type);

    double simulateFinalValue(double totalTime, int numSteps,
                              const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                              const std::vector<double>& ratePath) const override;

    std::vector<double> generatePath(double totalTime, int numSteps,
                                       const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                       const std::vector<double>& ratePath) const override;
};
