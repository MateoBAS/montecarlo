
#define BOOST_TEST_MODULE RiskCalculatorTests
#include <boost/test/unit_test.hpp>
#include <vector>
#include <stdexcept>

#include "RiskCalculator.h"

BOOST_AUTO_TEST_SUITE(RiskCalculator_Tests)

BOOST_AUTO_TEST_CASE(InvalidConfidenceLevelThrows) {
    std::vector<double> pnls = {1.0, 2.0, 3.0};
    RiskCalculator riskCalc(pnls);

    BOOST_CHECK_THROW(riskCalc.calculateVaR(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(1.05), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(-0.1), std::invalid_argument);

    BOOST_CHECK_THROW(riskCalc.calculateES(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateES(1.0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(VaR_Calculation) {
    std::vector<double> pnls = {10.0, -20.0, 100.0, -100.0, -50.0, 0.0, 20.0, -5.0, 50.0, -10.0};
    RiskCalculator riskCalc(pnls);

    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.90), -50.0, 1e-9);

    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.80), -20.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(ES_Calculation) {

    std::vector<double> pnls = {50.0, -100.0, -5.0, -50.0, 100.0, -20.0, -10.0, 10.0, 20.0, 0.0};
    RiskCalculator riskCalc(pnls);

    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.80), -75.0, 1e-9);

    double expectedES = (-100.0 - 50.0 - 20.0) / 3.0;
    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.70), expectedES, 1e-9);
}

BOOST_AUTO_TEST_CASE(ExtremeConfidenceLevels_ZeroIndex) {

    std::vector<double> pnls = {-500.0, -100.0, -50.0, 10.0, 50.0};
    RiskCalculator riskCalc(pnls);

    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.99), -500.0, 1e-9);
    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.99), -500.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(ES_IsNeverGreaterThanVaR) {
    std::vector<double> pnls = {10.0, -20.0, 100.0, -100.0, -50.0, 0.0, 20.0, -5.0, 50.0, -10.0};
    RiskCalculator riskCalc(pnls);

    for (double confidence : {0.80, 0.90, 0.95, 0.99}) {
        BOOST_CHECK_LE(riskCalc.calculateES(confidence), riskCalc.calculateVaR(confidence));
    }
}

BOOST_AUTO_TEST_SUITE_END()