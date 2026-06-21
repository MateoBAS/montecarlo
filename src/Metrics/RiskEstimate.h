#pragma once
#include <cmath>
#include <limits>

struct RiskEstimate {
    double value = 0.0;
    double standardError = std::numeric_limits<double>::quiet_NaN();

    bool hasStandardError() const {
        return std::isfinite(standardError);
    }
};
