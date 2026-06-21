#pragma once
#include "Asset.h"
#include <algorithm>

class BarrierOption : public Asset {
private:
    double underlyingInitialPrice;
    double strike;
    double barrier;
    double driftSpread;
    double volatility;

    double simulateBarrierPayoff(double totalTime, int numSteps,
                                 const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                 const std::vector<double>& ratePath) const;

public:
    BarrierOption(std::string name, double optionPremium, double underlyingInitialPrice,
                  double strike, double barrier, double drift, double volatility);

    double simulateFinalValue(double totalTime, int numSteps,
                              const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                              const std::vector<double>& ratePath) const override;

    std::vector<double> generatePath(double totalTime, int numSteps,
                                       const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                       const std::vector<double>& ratePath) const override;
};
