#define BOOST_TEST_MODULE StandardErrorTests
#include <boost/test/unit_test.hpp>

#include <cmath>
#include <vector>

#include "Metrics/MonteCarloErrorPolicy.h"
#include "Metrics/RiskCalculator.h"

BOOST_AUTO_TEST_SUITE(StandardError_Tests)

BOOST_AUTO_TEST_CASE(DisabledByDefault) {
    const std::vector<double> pnls = {1.0, -2.0, 3.0, -4.0, 5.0};
    RiskCalculator calc(pnls);

    const RiskEstimate var = calc.estimateVaR(0.80);
    BOOST_CHECK(!calc.standardErrorsEnabled());
    BOOST_CHECK(!var.hasStandardError());
}

BOOST_AUTO_TEST_CASE(IndependentMeanUsesCLT) {
    const std::vector<double> pnls = {1.0, 2.0, 3.0, 4.0, 5.0};

    StandardErrorOptions options;
    options.enabled = true;
    options.scheme = ErrorSampleScheme::Independent;

    RiskCalculator calc(pnls, options);
    const RiskEstimate mean = calc.estimateMeanPnL();

    BOOST_CHECK_CLOSE(mean.value, 3.0, 1e-12);
    BOOST_CHECK(mean.hasStandardError());
    BOOST_CHECK_CLOSE(mean.standardError,
                      MonteCarloErrorPolicy::meanStandardError(pnls), 1e-12);
}

BOOST_AUTO_TEST_CASE(AntitheticUsesPooledPairSamples) {

    const std::vector<double> paired = {-1000.0, 500.0, -800.0, 400.0, 0.0, 0.0};

    StandardErrorOptions options;
    options.enabled = true;
    options.scheme = ErrorSampleScheme::AntitheticPaired;

    RiskCalculator calc(paired, options);
    BOOST_CHECK_EQUAL(calc.effectiveSampleSize(), 6u);
    BOOST_CHECK_CLOSE(calc.calculateVaR(0.90), -1000.0, 1e-12);

    std::vector<double> averaged;
    averaged.reserve(3);
    for (size_t i = 0; i < paired.size(); i += 2) {
        averaged.push_back(0.5 * (paired[i] + paired[i + 1]));
    }
    RiskCalculator averagedCalc(averaged);
    BOOST_CHECK_LT(calc.calculateVaR(0.90), averagedCalc.calculateVaR(0.90));
}

BOOST_AUTO_TEST_CASE(SobolUsesBatchScheme) {
    std::vector<double> pnls;
    pnls.reserve(64);
    for (int i = 0; i < 64; ++i) {
        pnls.push_back(static_cast<double>(i % 7) - 3.0);
    }

    StandardErrorOptions options;
    options.enabled = true;
    options.scheme = ErrorSampleScheme::SobolBatchMeans;
    options.sobolBatchCount = 8;

    RiskCalculator calc(pnls, options);
    const RiskEstimate var = calc.estimateVaR(0.95);
    const RiskEstimate es = calc.estimateES(0.95);

    BOOST_CHECK(var.hasStandardError());
    BOOST_CHECK(es.hasStandardError());
    BOOST_CHECK(std::isfinite(var.standardError));
    BOOST_CHECK(std::isfinite(es.standardError));
}

BOOST_AUTO_TEST_CASE(ES_IsNeverGreaterThanVaR_OnLossScale) {
    const std::vector<double> pnls = {-100.0, -80.0, -50.0, -20.0, 0.0, 10.0, 20.0, 50.0};
    RiskCalculator calc(pnls);

    const double var95 = calc.calculateVaR(0.95);
    const double es95 = calc.calculateES(0.95);
    BOOST_CHECK_LE(es95, var95);
}

BOOST_AUTO_TEST_CASE(AntitheticPairedBootstrapIsPositive) {
    std::vector<double> paired;
    paired.reserve(200);
    for (int i = 0; i < 100; ++i) {
        const double shock = static_cast<double>(i - 50);
        paired.push_back(-500.0 + 20.0 * shock);
        paired.push_back(-500.0 - 15.0 * shock);
    }

    StandardErrorOptions options;
    options.enabled = true;
    options.scheme = ErrorSampleScheme::AntitheticPaired;
    options.bootstrapReplications = 128;

    RiskCalculator calc(paired, options);
    const RiskEstimate var = calc.estimateVaR(0.95);
    const RiskEstimate es = calc.estimateES(0.95);

    BOOST_CHECK(var.hasStandardError());
    BOOST_CHECK(es.hasStandardError());
    BOOST_CHECK_GT(var.standardError, 0.0);
    BOOST_CHECK_GT(es.standardError, 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
