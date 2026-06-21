#ifndef RISK_CALCULATOR_H
#define RISK_CALCULATOR_H

#include <span>
#include <vector>

#include "MonteCarloErrorPolicy.h"
#include "RiskEstimate.h"

class RiskCalculator {
private:
    std::vector<double> rawPnLs;
    std::vector<double> metricPnLs;
    std::vector<double> sortedPnLs;
    StandardErrorOptions errorOptions;
    std::vector<double> errorSamples;

    void prepareMetricSamples();

public:
    RiskCalculator(const std::vector<double>& simulatedPnLs);
    explicit RiskCalculator(std::span<const double> simulatedPnLs);
    RiskCalculator(std::span<const double> simulatedPnLs, StandardErrorOptions options);

    bool standardErrorsEnabled() const { return errorOptions.enabled; }
    ErrorSampleScheme sampleScheme() const { return errorOptions.scheme; }
    std::size_t effectiveSampleSize() const { return metricPnLs.size(); }

    double calculateVaR(double confidenceLevel) const;
    double calculateES(double confidenceLevel) const;

    RiskEstimate estimateVaR(double confidenceLevel) const;
    RiskEstimate estimateES(double confidenceLevel) const;
    RiskEstimate estimateMeanPnL() const;
};

#endif // RISK_CALCULATOR_H
