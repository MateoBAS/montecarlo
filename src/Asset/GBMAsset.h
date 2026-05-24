#pragma once
#include "Asset.h"

class GBMAsset : public Asset {
private:
    double driftSpread;
    double volatility;

public:
    GBMAsset(std::string name, double initialPrice, double drift, double volatility);

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const std::vector<double>& Zs,
                                     const std::vector<double>& ratePath) const override;
};