#include "RiskCalculator.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "RiskQuantile.h"

RiskCalculator::RiskCalculator(const std::vector<double>& simulatedPnLs)
    : RiskCalculator(std::span<const double>(simulatedPnLs)) {}

RiskCalculator::RiskCalculator(std::span<const double> simulatedPnLs)
    : RiskCalculator(simulatedPnLs, StandardErrorOptions{}) {}

RiskCalculator::RiskCalculator(std::span<const double> simulatedPnLs,
                               StandardErrorOptions options)
    : rawPnLs(simulatedPnLs.begin(), simulatedPnLs.end()),
      errorOptions(options) {
    if (rawPnLs.empty()) {
        throw std::invalid_argument("RiskCalculator requires at least one simulated PnL.");
    }

    prepareMetricSamples();
    sortedPnLs = metricPnLs;
    std::ranges::sort(sortedPnLs);
}

void RiskCalculator::prepareMetricSamples() {
    metricPnLs = rawPnLs;
    if (errorOptions.enabled) {
        errorSamples = metricPnLs;
    }
}

double RiskCalculator::calculateVaR(double confidenceLevel) const {
    return estimateVaR(confidenceLevel).value;
}

double RiskCalculator::calculateES(double confidenceLevel) const {
    return estimateES(confidenceLevel).value;
}

RiskEstimate RiskCalculator::estimateVaR(double confidenceLevel) const {
    if (confidenceLevel <= 0.0 || confidenceLevel >= 1.0) {
        throw std::invalid_argument("El nivel de confianza debe estar entre 0 y 1 (ej. 0.99)");
    }

    const std::size_t index =
        RiskMetrics::empiricalQuantileIndex(sortedPnLs.size(), confidenceLevel);

    RiskEstimate result;
    result.value = sortedPnLs[index];

    if (!errorOptions.enabled) {
        return result;
    }

    if (errorOptions.scheme == ErrorSampleScheme::SobolBatchMeans) {
        result.standardError = MonteCarloErrorPolicy::batchQuantileStandardError(
            rawPnLs, confidenceLevel, errorOptions.sobolBatchCount);
    } else if (errorOptions.scheme == ErrorSampleScheme::AntitheticPaired) {
        result.standardError = MonteCarloErrorPolicy::pairedVarStandardError(
            errorSamples, confidenceLevel, errorOptions.bootstrapReplications);
    } else {
        result.standardError = MonteCarloErrorPolicy::varStandardError(
            errorSamples, confidenceLevel, errorOptions.bootstrapReplications);
    }

    return result;
}

RiskEstimate RiskCalculator::estimateES(double confidenceLevel) const {
    if (confidenceLevel <= 0.0 || confidenceLevel >= 1.0) {
        throw std::invalid_argument("El nivel de confianza debe estar entre 0 y 1 (ej. 0.99)");
    }

    const std::size_t index =
        RiskMetrics::empiricalQuantileIndex(sortedPnLs.size(), confidenceLevel);

    RiskEstimate result;
    if (index == 0) {
        result.value = sortedPnLs[0];
    } else {
        result.value = std::reduce(sortedPnLs.begin(),
                                   sortedPnLs.begin() + static_cast<std::ptrdiff_t>(index),
                                   0.0) /
                       static_cast<double>(index);
    }

    if (!errorOptions.enabled) {
        return result;
    }

    if (errorOptions.scheme == ErrorSampleScheme::SobolBatchMeans) {
        result.standardError = MonteCarloErrorPolicy::batchEsStandardError(
            rawPnLs, confidenceLevel, errorOptions.sobolBatchCount);
    } else if (errorOptions.scheme == ErrorSampleScheme::AntitheticPaired) {
        result.standardError = MonteCarloErrorPolicy::pairedEsStandardError(
            errorSamples, confidenceLevel, errorOptions.bootstrapReplications);
    } else {
        result.standardError = MonteCarloErrorPolicy::esStandardError(
            errorSamples, confidenceLevel, errorOptions.bootstrapReplications);
    }

    return result;
}

RiskEstimate RiskCalculator::estimateMeanPnL() const {
    RiskEstimate result;
    result.value = std::reduce(metricPnLs.begin(), metricPnLs.end(), 0.0) /
                   static_cast<double>(metricPnLs.size());

    if (!errorOptions.enabled) {
        return result;
    }

    if (errorOptions.scheme == ErrorSampleScheme::SobolBatchMeans) {
        result.standardError = MonteCarloErrorPolicy::batchMeanStandardError(
            rawPnLs, errorOptions.sobolBatchCount);
    } else if (errorOptions.scheme == ErrorSampleScheme::AntitheticPaired) {
        result.standardError = MonteCarloErrorPolicy::pairedMeanStandardError(
            errorSamples, errorOptions.bootstrapReplications);
    } else {
        result.standardError = MonteCarloErrorPolicy::meanStandardError(errorSamples);
    }

    return result;
}
