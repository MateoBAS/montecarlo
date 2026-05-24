#pragma once
#include <vector>

class InterestRateModel {
public:
    virtual ~InterestRateModel() = default;

    virtual std::vector<double> simulateShortRatePath(double totalTime, int numSteps,
                                                      const std::vector<double>& Zs) const = 0;

    virtual double discountFactorFromPath(const std::vector<double>& ratePath, double dt) const = 0;

    virtual double zeroCouponBondPrice(double t, double T, double shortRateAtT) const = 0;

    virtual double initialShortRate() const = 0;
};
