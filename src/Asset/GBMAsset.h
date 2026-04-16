#pragma once
#include "Asset.h"

class GBMAsset : public Asset {
private:
    double drift;
    double volatility;

public:
    GBMAsset(std::string name, double initialPrice, double drift, double volatility);

    std::vector<double> generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const override;
};