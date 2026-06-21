#include "MonteCarloErrorPolicy.h"
#include "RiskQuantile.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <numbers>

namespace {

double empiricalQuantile(std::span<const double> sortedSamples, double confidenceLevel) {
    if (sortedSamples.empty()) {
        throw std::invalid_argument("Cannot estimate quantile from empty sample.");
    }
    const std::size_t index =
        RiskMetrics::empiricalQuantileIndex(sortedSamples.size(), confidenceLevel);
    return sortedSamples[index];
}

}  // namespace

std::vector<double> MonteCarloErrorPolicy::prepareSamples(std::span<const double> pnls,
                                                          ErrorSampleScheme scheme) {
    (void)scheme;
    // Las muestras ya llegan listas para métricas:
    // - Independent / AntitheticPaired: PnL en bruto (antitéticas: [Z1,-Z1,Z2,-Z2,...])
    // - SobolBatchMeans: PnL crudo (el error usa lotes aparte)
    return {pnls.begin(), pnls.end()};
}

double MonteCarloErrorPolicy::sampleStdDev(std::span<const double> samples) {
    if (samples.size() < 2) {
        return 0.0;
    }

    const double mean = std::reduce(samples.begin(), samples.end(), 0.0) /
                        static_cast<double>(samples.size());
    double sumSq = 0.0;
    for (double value : samples) {
        const double diff = value - mean;
        sumSq += diff * diff;
    }
    return std::sqrt(sumSq / static_cast<double>(samples.size() - 1));
}

double MonteCarloErrorPolicy::meanStandardError(std::span<const double> samples) {
    if (samples.empty()) {
        throw std::invalid_argument("Cannot compute mean standard error from empty sample.");
    }
    return sampleStdDev(samples) / std::sqrt(static_cast<double>(samples.size()));
}

double MonteCarloErrorPolicy::kernelDensityAt(std::span<const double> samples, double x) {
    if (samples.size() < 2) {
        return 0.0;
    }

    const double stdDev = sampleStdDev(samples);
    const double n = static_cast<double>(samples.size());
    const double bandwidth = std::max(1e-6, 1.06 * stdDev * std::pow(n, -0.2));

    double sum = 0.0;
    for (double sample : samples) {
        const double u = (x - sample) / bandwidth;
        sum += std::exp(-0.5 * u * u);
    }

    return sum / (n * bandwidth * std::sqrt(2.0 * std::numbers::pi));
}

double MonteCarloErrorPolicy::empiricalVaR(std::span<const double> samples, double confidenceLevel) {
    std::vector<double> sorted(samples.begin(), samples.end());
    std::ranges::sort(sorted);
    return empiricalQuantile(sorted, confidenceLevel);
}

double MonteCarloErrorPolicy::empiricalES(std::span<const double> samples, double confidenceLevel) {
    std::vector<double> sorted(samples.begin(), samples.end());
    std::ranges::sort(sorted);

    const std::size_t index =
        RiskMetrics::empiricalQuantileIndex(sorted.size(), confidenceLevel);
    if (index == 0) {
        return sorted.front();
    }

    return std::reduce(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(index), 0.0) /
           static_cast<double>(index);
}

double MonteCarloErrorPolicy::varStandardError(std::span<const double> samples,
                                               double confidenceLevel,
                                               int bootstrapReplications) {
    if (samples.size() < 2 || bootstrapReplications < 2) {
        return 0.0;
    }

    std::vector<double> bootstrapEstimates(static_cast<size_t>(bootstrapReplications));
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, samples.size() - 1);

    for (int b = 0; b < bootstrapReplications; ++b) {
        std::vector<double> resampled;
        resampled.reserve(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            resampled.push_back(samples[dist(rng)]);
        }
        bootstrapEstimates[static_cast<size_t>(b)] = empiricalVaR(resampled, confidenceLevel);
    }

    return sampleStdDev(bootstrapEstimates);
}

double MonteCarloErrorPolicy::quantileStandardError(std::span<const double> samples,
                                                    double confidenceLevel) {
    return varStandardError(samples, confidenceLevel, 256);
}

double MonteCarloErrorPolicy::esStandardError(std::span<const double> samples,
                                             double confidenceLevel,
                                             int bootstrapReplications) {
    if (samples.size() < 2 || bootstrapReplications < 2) {
        return 0.0;
    }

    std::vector<double> bootstrapEstimates(static_cast<size_t>(bootstrapReplications));
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, samples.size() - 1);

    for (int b = 0; b < bootstrapReplications; ++b) {
        std::vector<double> resampled;
        resampled.reserve(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            resampled.push_back(samples[dist(rng)]);
        }
        bootstrapEstimates[static_cast<size_t>(b)] = empiricalES(resampled, confidenceLevel);
    }

    return sampleStdDev(bootstrapEstimates);
}

namespace {

std::vector<double> resampleAntitheticPairs(std::span<const double> pairedPnls,
                                            std::mt19937& rng) {
    if (pairedPnls.size() < 2 || pairedPnls.size() % 2 != 0) {
        throw std::invalid_argument("Antithetic samples must contain an even number of PnLs.");
    }

    const size_t pairCount = pairedPnls.size() / 2;
    std::uniform_int_distribution<size_t> dist(0, pairCount - 1);

    std::vector<double> resampled;
    resampled.reserve(pairedPnls.size());
    for (size_t i = 0; i < pairCount; ++i) {
        const size_t pairIndex = dist(rng);
        const size_t base = pairIndex * 2;
        resampled.push_back(pairedPnls[base]);
        resampled.push_back(pairedPnls[base + 1]);
    }
    return resampled;
}

}  // namespace

double MonteCarloErrorPolicy::pairedVarStandardError(std::span<const double> pairedPnls,
                                                     double confidenceLevel,
                                                     int bootstrapReplications) {
    if (pairedPnls.size() < 4 || pairedPnls.size() % 2 != 0 || bootstrapReplications < 2) {
        return 0.0;
    }

    std::vector<double> bootstrapEstimates(static_cast<size_t>(bootstrapReplications));
    std::mt19937 rng(42);

    for (int b = 0; b < bootstrapReplications; ++b) {
        const std::vector<double> resampled = resampleAntitheticPairs(pairedPnls, rng);
        bootstrapEstimates[static_cast<size_t>(b)] =
            empiricalVaR(resampled, confidenceLevel);
    }

    return sampleStdDev(bootstrapEstimates);
}

double MonteCarloErrorPolicy::pairedEsStandardError(std::span<const double> pairedPnls,
                                                    double confidenceLevel,
                                                    int bootstrapReplications) {
    if (pairedPnls.size() < 4 || pairedPnls.size() % 2 != 0 || bootstrapReplications < 2) {
        return 0.0;
    }

    std::vector<double> bootstrapEstimates(static_cast<size_t>(bootstrapReplications));
    std::mt19937 rng(42);

    for (int b = 0; b < bootstrapReplications; ++b) {
        const std::vector<double> resampled = resampleAntitheticPairs(pairedPnls, rng);
        bootstrapEstimates[static_cast<size_t>(b)] =
            empiricalES(resampled, confidenceLevel);
    }

    return sampleStdDev(bootstrapEstimates);
}

double MonteCarloErrorPolicy::pairedMeanStandardError(std::span<const double> pairedPnls,
                                                      int bootstrapReplications) {
    if (pairedPnls.size() < 4 || pairedPnls.size() % 2 != 0 || bootstrapReplications < 2) {
        return 0.0;
    }

    std::vector<double> bootstrapEstimates(static_cast<size_t>(bootstrapReplications));
    std::mt19937 rng(42);

    for (int b = 0; b < bootstrapReplications; ++b) {
        const std::vector<double> resampled = resampleAntitheticPairs(pairedPnls, rng);
        bootstrapEstimates[static_cast<size_t>(b)] =
            std::reduce(resampled.begin(), resampled.end(), 0.0) /
            static_cast<double>(resampled.size());
    }

    return sampleStdDev(bootstrapEstimates);
}

double MonteCarloErrorPolicy::batchQuantileStandardError(std::span<const double> pnls,
                                                         double confidenceLevel,
                                                         int batchCount) {
    if (pnls.size() < 2 || batchCount < 2) {
        return 0.0;
    }

    const size_t batchSize = pnls.size() / static_cast<size_t>(batchCount);
    if (batchSize < 2) {
        return 0.0;
    }

    std::vector<double> batchEstimates(static_cast<size_t>(batchCount));
    for (int batch = 0; batch < batchCount; ++batch) {
        const size_t begin = static_cast<size_t>(batch) * batchSize;
        const size_t end = begin + batchSize;
        batchEstimates[static_cast<size_t>(batch)] =
            empiricalVaR(pnls.subspan(begin, end - begin), confidenceLevel);
    }

    return sampleStdDev(batchEstimates) / std::sqrt(static_cast<double>(batchCount));
}

double MonteCarloErrorPolicy::batchEsStandardError(std::span<const double> pnls,
                                                   double confidenceLevel,
                                                   int batchCount) {
    if (pnls.size() < 2 || batchCount < 2) {
        return 0.0;
    }

    const size_t batchSize = pnls.size() / static_cast<size_t>(batchCount);
    if (batchSize < 2) {
        return 0.0;
    }

    std::vector<double> batchEstimates(static_cast<size_t>(batchCount));
    for (int batch = 0; batch < batchCount; ++batch) {
        const size_t begin = static_cast<size_t>(batch) * batchSize;
        const size_t end = begin + batchSize;
        batchEstimates[static_cast<size_t>(batch)] =
            empiricalES(pnls.subspan(begin, end - begin), confidenceLevel);
    }

    return sampleStdDev(batchEstimates) / std::sqrt(static_cast<double>(batchCount));
}

double MonteCarloErrorPolicy::batchMeanStandardError(std::span<const double> pnls, int batchCount) {
    if (pnls.size() < 2 || batchCount < 2) {
        return 0.0;
    }

    const size_t batchSize = pnls.size() / static_cast<size_t>(batchCount);
    if (batchSize < 1) {
        return 0.0;
    }

    std::vector<double> batchMeans(static_cast<size_t>(batchCount));
    for (int batch = 0; batch < batchCount; ++batch) {
        const size_t begin = static_cast<size_t>(batch) * batchSize;
        const size_t end = begin + batchSize;
        batchMeans[static_cast<size_t>(batch)] =
            std::reduce(pnls.begin() + static_cast<std::ptrdiff_t>(begin),
                        pnls.begin() + static_cast<std::ptrdiff_t>(end),
                        0.0) /
            static_cast<double>(batchSize);
    }

    return sampleStdDev(batchMeans) / std::sqrt(static_cast<double>(batchCount));
}
