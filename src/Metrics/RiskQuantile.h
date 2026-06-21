#pragma once
#include <cmath>
#include <cstddef>

namespace RiskMetrics {

// Índice del cuantil empírico: floor(n * alpha), con alpha = 1 - confidence.
// Evita el bug de static_cast<size_t>(n * alpha) cuando alpha no es exacto en binario
// (p.ej. 10 * 0.1 → 0.999... → índice 0 en lugar de 1).
inline std::size_t empiricalQuantileIndex(std::size_t sampleCount, double confidenceLevel) {
    const double alpha = 1.0 - confidenceLevel;
    return static_cast<std::size_t>(
        std::floor(alpha * static_cast<double>(sampleCount) + 1e-9));
}

}  // namespace RiskMetrics
