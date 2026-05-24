#pragma once
#include <vector>

class YieldCurve {
private:
    std::vector<double> times;
    std::vector<double> zeroRates;

public:
    YieldCurve() = default;
    YieldCurve(std::vector<double> times, std::vector<double> zeroRates);

    double zeroRate(double t) const;
    double discountFactor(double t) const;
    double forwardRate(double t) const;
    double forwardRateDerivative(double t) const;
};
