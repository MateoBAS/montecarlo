#pragma once
#include "InterestRateModel.h"
#include "YieldCurve.h"
#include <limits>

class HullWhiteModel : public InterestRateModel {
public:
    HullWhiteModel(double meanReversion, double volatility, YieldCurve curve,
                   double initialShortRate = std::numeric_limits<double>::quiet_NaN());

    std::vector<double> simulateShortRatePath(double totalTime, int numSteps,
                                              const std::vector<double>& Zs) const override;

    double discountFactorFromPath(const std::vector<double>& ratePath, double dt) const override;

    double zeroCouponBondPrice(double t, double T, double shortRateAtT) const override;

    double initialShortRate() const override;

    const YieldCurve& getCurve() const { return curve; }

private:
    double meanReversion;
    double volatility;
    YieldCurve curve;
    double r0;

    double theta(double t) const;
};
