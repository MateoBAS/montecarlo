#include "HullWhiteModel.h"
#include <cmath>
#include <stdexcept>

HullWhiteModel::HullWhiteModel(double meanReversionIn, double volatilityIn, YieldCurve curveIn,
                               double initialShortRate)
    : meanReversion(meanReversionIn), volatility(volatilityIn), curve(std::move(curveIn)), r0(0.0) {
    if (meanReversion <= 0.0) {
        throw std::invalid_argument("HullWhiteModel: meanReversion must be positive.");
    }
    if (volatility < 0.0) {
        throw std::invalid_argument("HullWhiteModel: volatility must be non-negative.");
    }

    if (std::isnan(initialShortRate)) {
        r0 = curve.forwardRate(0.0);
    } else {
        r0 = initialShortRate;
    }
}

double HullWhiteModel::theta(double t) const {
    double f0 = curve.forwardRate(t);
    double df0 = curve.forwardRateDerivative(t);
    double expTerm = std::exp(-2.0 * meanReversion * t);
    double sigmaAdj = (volatility * volatility / (2.0 * meanReversion)) * (1.0 - expTerm);

    return df0 + meanReversion * f0 + sigmaAdj;
}

std::vector<double> HullWhiteModel::simulateShortRatePath(double totalTime, int numSteps,
                                                          const std::vector<double>& Zs) const {
    if (numSteps <= 0) {
        return {r0};
    }
    if (static_cast<int>(Zs.size()) != numSteps) {
        throw std::invalid_argument("HullWhiteModel: Zs size must match numSteps.");
    }

    std::vector<double> path;
    path.reserve(static_cast<size_t>(numSteps) + 1);

    double dt = totalTime / numSteps;
    double currentRate = r0;
    path.push_back(currentRate);

    for (int i = 0; i < numSteps; ++i) {
        double t = i * dt;
        double drift = (theta(t) - meanReversion * currentRate) * dt;
        double diffusion = volatility * std::sqrt(dt) * Zs[i];
        currentRate += drift + diffusion;
        path.push_back(currentRate);
    }

    return path;
}

double HullWhiteModel::discountFactorFromPath(const std::vector<double>& ratePath, double dt) const {
    if (ratePath.empty()) {
        return 1.0;
    }
    if (ratePath.size() == 1) {
        return std::exp(-ratePath.front() * dt);
    }

    double integral = 0.0;
    for (size_t i = 0; i + 1 < ratePath.size(); ++i) {
        integral += 0.5 * (ratePath[i] + ratePath[i + 1]) * dt;
    }

    return std::exp(-integral);
}

double HullWhiteModel::zeroCouponBondPrice(double t, double T, double shortRateAtT) const {
    if (T <= t) {
        return 1.0;
    }

    double P0T = curve.discountFactor(T);
    double P0t = curve.discountFactor(t);
    double f0t = curve.forwardRate(t);
    double B = (1.0 - std::exp(-meanReversion * (T - t))) / meanReversion;

    double sigma2 = volatility * volatility;
    double expTerm = std::exp(-2.0 * meanReversion * t);
    double varianceAdj = (sigma2 / (4.0 * meanReversion)) * (1.0 - expTerm) * B * B;

    double A = (P0T / P0t) * std::exp(B * f0t - varianceAdj);

    return A * std::exp(-B * shortRateAtT);
}

double HullWhiteModel::initialShortRate() const {
    return r0;
}
