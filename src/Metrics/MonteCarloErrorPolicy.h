#pragma once
#include <span>
#include <vector>

enum class ErrorSampleScheme {
    Independent,
    AntitheticPaired,
    SobolBatchMeans
};

struct StandardErrorOptions {
    bool enabled = false;
    ErrorSampleScheme scheme = ErrorSampleScheme::Independent;
    int bootstrapReplications = 256;
    int sobolBatchCount = 32;
};

class MonteCarloErrorPolicy {
public:
    static std::vector<double> prepareSamples(std::span<const double> pnls, ErrorSampleScheme scheme);

    static double meanStandardError(std::span<const double> samples);

    static double quantileStandardError(std::span<const double> samples, double confidenceLevel);

    static double varStandardError(std::span<const double> samples, double confidenceLevel,
                                   int bootstrapReplications);

    static double esStandardError(std::span<const double> samples, double confidenceLevel,
                                  int bootstrapReplications);

    static double pairedVarStandardError(std::span<const double> pairedPnls, double confidenceLevel,
                                         int bootstrapReplications);

    static double pairedEsStandardError(std::span<const double> pairedPnls, double confidenceLevel,
                                        int bootstrapReplications);

    static double pairedMeanStandardError(std::span<const double> pairedPnls,
                                          int bootstrapReplications);

    static double batchQuantileStandardError(std::span<const double> pnls, double confidenceLevel,
                                             int batchCount);

    static double batchEsStandardError(std::span<const double> pnls, double confidenceLevel,
                                       int batchCount);

    static double batchMeanStandardError(std::span<const double> pnls, int batchCount);

private:
    static double empiricalVaR(std::span<const double> samples, double confidenceLevel);
    static double empiricalES(std::span<const double> samples, double confidenceLevel);
    static double sampleStdDev(std::span<const double> samples);
    static double kernelDensityAt(std::span<const double> samples, double x);
};
