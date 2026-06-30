#pragma once
#include <cmath>
#include <cstddef>

namespace RiskMetrics {

inline std::size_t empiricalQuantileIndex(std::size_t sampleCount, double confidenceLevel) {
    const double alpha = 1.0 - confidenceLevel;
    return static_cast<std::size_t>(
        std::floor(alpha * static_cast<double>(sampleCount) + 1e-9));
}

}
