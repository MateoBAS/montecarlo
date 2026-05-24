#include "YieldCurve.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

YieldCurve::YieldCurve(std::vector<double> timesIn, std::vector<double> zeroRatesIn)
    : times(std::move(timesIn)), zeroRates(std::move(zeroRatesIn)) {
    if (times.size() != zeroRates.size()) {
        throw std::invalid_argument("YieldCurve: times and zeroRates must have the same size.");
    }
    if (times.empty()) {
        return;
    }
    for (size_t i = 1; i < times.size(); ++i) {
        if (times[i] <= times[i - 1]) {
            throw std::invalid_argument("YieldCurve: times must be strictly increasing.");
        }
    }
}

double YieldCurve::zeroRate(double t) const {
    if (times.empty()) {
        return 0.0;
    }
    if (t <= times.front()) {
        return zeroRates.front();
    }
    if (t >= times.back()) {
        return zeroRates.back();
    }

    auto upper = std::upper_bound(times.begin(), times.end(), t);
    size_t idx = static_cast<size_t>(std::distance(times.begin(), upper));
    size_t i0 = idx - 1;
    size_t i1 = idx;

    double t0 = times[i0];
    double t1 = times[i1];
    double r0 = zeroRates[i0];
    double r1 = zeroRates[i1];
    double w = (t - t0) / (t1 - t0);

    return r0 + w * (r1 - r0);
}

double YieldCurve::discountFactor(double t) const {
    if (t <= 0.0) {
        return 1.0;
    }
    double r = zeroRate(t);
    return std::exp(-r * t);
}

double YieldCurve::forwardRate(double t) const {
    const double eps = 1e-4;
    if (t < eps) {
        double p0 = discountFactor(0.0);
        double p1 = discountFactor(eps);
        return -(std::log(p1) - std::log(p0)) / eps;
    }

    double t1 = std::max(0.0, t - eps);
    double t2 = t + eps;
    double p1 = discountFactor(t1);
    double p2 = discountFactor(t2);

    return -(std::log(p2) - std::log(p1)) / (t2 - t1);
}

double YieldCurve::forwardRateDerivative(double t) const {
    const double eps = 1e-4;
    double t1 = std::max(0.0, t - eps);
    double t2 = t + eps;
    double f1 = forwardRate(t1);
    double f2 = forwardRate(t2);
    return (f2 - f1) / (t2 - t1);
}
