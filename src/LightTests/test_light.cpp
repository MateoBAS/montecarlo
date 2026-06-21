#define BOOST_TEST_MODULE LightSimulationTests
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <memory>
#include <vector>

#include <Eigen/Dense>

#include "Asset/GBMAsset.h"
#include "Asset/EuropeanOption.h"
#include "Asset/BarrierOption.h"
#include "Asset/CouponBond.h"
#include "Portfolio/Portfolio.h"

static Eigen::RowVectorXd makeRowVector(const std::vector<double>& values) {
    Eigen::RowVectorXd row(static_cast<Eigen::Index>(values.size()));
    for (Eigen::Index i = 0; i < row.size(); ++i) {
        row[i] = values[static_cast<size_t>(i)];
    }
    return row;
}

static void checkFinalMatchesPath(const Asset& asset, double totalTime, int numSteps,
                                  const Eigen::RowVectorXd& zRow,
                                  const std::vector<double>& ratePath) {
    const double terminal = asset.simulateFinalValue(totalTime, numSteps, zRow, ratePath);
    const std::vector<double> path = asset.generatePath(totalTime, numSteps, zRow, ratePath);
    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK_CLOSE(terminal, path.back(), 1e-10);
}

BOOST_AUTO_TEST_SUITE(LightSimulation_Tests)

BOOST_AUTO_TEST_CASE(FinalValueMatchesGeneratePathBack) {
    const std::vector<double> zs = {0.0, 0.5};
    const int numSteps = static_cast<int>(zs.size());
    const std::vector<double> ratePath = {0.03, 0.02, 0.01};
    const auto zRow = makeRowVector(zs);

    GBMAsset stock("Stock", 100.0, 0.05, 0.20);
    EuropeanOption call("Call", 5.0, 100.0, 100.0, 0.05, 0.20, OptionType::Call);
    BarrierOption barrier("Barrier", 5.0, 100.0, 100.0, 150.0, 0.05, 0.20);
    CouponBond bond("Bond", 1000.0, 0.05, 2.0, 0.05, 0.10);

    checkFinalMatchesPath(stock, 1.0, numSteps, zRow, ratePath);
    checkFinalMatchesPath(call, 1.0, numSteps, zRow, ratePath);
    checkFinalMatchesPath(barrier, 1.0, numSteps, zRow, ratePath);
    checkFinalMatchesPath(bond, 1.0, 1, makeRowVector({0.0}), {0.0, 0.0});
}

BOOST_AUTO_TEST_CASE(GeneratePathShapeDependsOnAsset) {
    const std::vector<double> zs = {0.0, 0.0, 0.0};
    const int numSteps = static_cast<int>(zs.size());
    const std::vector<double> ratePath(numSteps + 1, 0.0);
    const auto zRow = makeRowVector(zs);

    GBMAsset stock("Stock", 100.0, 0.05, 0.0);
    EuropeanOption call("Call", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Call);

    const auto stockPath = stock.generatePath(1.0, numSteps, zRow, ratePath);
    const auto callPath = call.generatePath(1.0, numSteps, zRow, ratePath);

    BOOST_CHECK(stock.recordsIntermediatePrices());
    BOOST_CHECK(!call.recordsIntermediatePrices());
    BOOST_CHECK_EQUAL(stockPath.size(), static_cast<size_t>(numSteps + 1));
    BOOST_CHECK_EQUAL(callPath.size(), 2u);
}

BOOST_AUTO_TEST_CASE(EuropeanDeterministicPayoffWithoutRatePath) {
    EuropeanOption call("Call", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Call);
    const std::vector<double> emptyRatePath;
    const double payoff = call.simulateFinalValue(1.0, 1, makeRowVector({0.0}), emptyRatePath);
    const double expected = 100.0 * std::exp(0.10) - 100.0;
    BOOST_CHECK_CLOSE(payoff, expected, 1e-10);
}

BOOST_AUTO_TEST_CASE(BarrierKnockOutIsZero) {
    BarrierOption barrier("Barrier", 5.0, 100.0, 100.0, 120.0, 0.0, 0.20);
    const std::vector<double> ratePath(3, 0.0);
    const double payoff = barrier.simulateFinalValue(1.0, 2, makeRowVector({5.0, -1.0}), ratePath);
    BOOST_CHECK_SMALL(payoff, 1e-12);
}

BOOST_AUTO_TEST_CASE(PortfolioUsesFinalValuePath) {
    Portfolio port;
    port.addPosition(std::make_shared<GBMAsset>("Stock", 100.0, 0.05, 0.0), 2.0);

    const int numSteps = 2;
    Eigen::Matrix<double, 1, Eigen::Dynamic> zMatrix(1, numSteps);
    zMatrix.setZero();
    const std::vector<double> ratePath(numSteps + 1, 0.0);

    const double pnl = port.simulatePathPnL(1.0, numSteps, zMatrix, ratePath, nullptr);
    const double expectedFinal = 100.0 * std::exp(0.05);
    const double expectedPnL = 2.0 * (expectedFinal - 100.0);
    BOOST_CHECK_CLOSE(pnl, expectedPnL, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
