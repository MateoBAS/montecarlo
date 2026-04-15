#pragma once
#include "Asset.h"
#include <algorithm>

enum class OptionType { Call, Put };

class EuropeanOption : public Asset {
private:
    double underlyingInitialPrice;
    double strike;
    double drift;
    double volatility;
    OptionType type;

public:
    EuropeanOption(std::string name, double optionPremium, double underlyingInitialPrice, 
                   double strike, double drift, double volatility, OptionType type);

    std::vector<double> generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const override;
};