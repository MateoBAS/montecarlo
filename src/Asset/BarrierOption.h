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

public:
    BarrierOption(std::string name, double optionPremium, double underlyingInitialPrice, 
                  double strike, double barrier, double drift, double volatility);

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const std::vector<double>& Zs,
                                     const std::vector<double>& ratePath) const override;
};