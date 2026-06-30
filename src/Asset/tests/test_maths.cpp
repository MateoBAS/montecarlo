
#define BOOST_TEST_MODULE AssetTests
#include <boost/test/unit_test.hpp>
#include <vector>
#include <cmath>

#include <Eigen/Dense>

#include "Asset/GBMAsset.h"
#include "Asset/EuropeanOption.h"
#include "Asset/BarrierOption.h"
#include "Asset/CouponBond.h"

static Eigen::RowVectorXd makeRowVector(const std::vector<double>& values) {
    Eigen::RowVectorXd row(static_cast<Eigen::Index>(values.size()));
    for (Eigen::Index i = 0; i < row.size(); ++i) {
        row[i] = values[static_cast<size_t>(i)];
    }
    return row;
}

BOOST_AUTO_TEST_SUITE(GBMAsset_Tests)

BOOST_AUTO_TEST_CASE(DeterministicGrowth_ZeroVol) {

    GBMAsset stock("StockA", 100.0, 0.05, 0.0);

    std::vector<double> Zs = {0.0, 0.0, 0.0};
    double totalTime = 1.0;
    int numSteps = 3;
    std::vector<double> ratePath(numSteps + 1, 0.0);

    std::vector<double> path = stock.generatePath(totalTime, numSteps, makeRowVector(Zs), ratePath);

    BOOST_CHECK_EQUAL(path.size(), 4);
    BOOST_CHECK_EQUAL(path[0], 100.0);

    double expectedFinalPrice = 100.0 * std::exp(0.05 * 1.0);
    BOOST_CHECK_CLOSE(path.back(), expectedFinalPrice, 1e-7);
}

BOOST_AUTO_TEST_CASE(StochasticMovement) {

    GBMAsset stock("StockB", 100.0, 0.0, 0.20);

    std::vector<double> Zs = {1.0};
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = stock.generatePath(1.0, 1, makeRowVector(Zs), ratePath);

    double expectedPrice = 100.0 * std::exp(0.18);
    BOOST_CHECK_CLOSE(path.back(), expectedPrice, 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(EuropeanOption_Tests)

BOOST_AUTO_TEST_CASE(CallOption_InTheMoney) {

    EuropeanOption call("Call_ITM", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Call);
    std::vector<double> Zs = {0.0};
    std::vector<double> ratePath(2, 0.0);

    std::vector<double> path = call.generatePath(1.0, 1, makeRowVector(Zs), ratePath);

    double expectedPayoff = 100.0 * std::exp(0.10) - 100.0;

    BOOST_CHECK_EQUAL(path[0], 5.0);
    BOOST_CHECK_CLOSE(path.back(), expectedPayoff, 1e-7);
}

BOOST_AUTO_TEST_CASE(PutOption_OutOfTheMoney) {

    EuropeanOption put("Put_OTM", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Put);
    std::vector<double> Zs = {0.0};
    std::vector<double> ratePath(2, 0.0);

    std::vector<double> path = put.generatePath(1.0, 1, makeRowVector(Zs), ratePath);

    BOOST_CHECK_SMALL(path.back(), 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(BarrierOption_Tests)

BOOST_AUTO_TEST_CASE(UpAndOut_NoKnockOut) {

    BarrierOption barrierOpt("Barrier_Safe", 5.0, 100.0, 100.0, 120.0, 0.0, 0.20);

    std::vector<double> Zs = {0.5, 0.5};
    std::vector<double> ratePath(3, 0.0);
    std::vector<double> path = barrierOpt.generatePath(1.0, 2, makeRowVector(Zs), ratePath);

    BOOST_CHECK(path.back() > 0.0);
}

BOOST_AUTO_TEST_CASE(UpAndOut_KnocksOut) {

    BarrierOption barrierOpt("Barrier_Dead", 5.0, 100.0, 100.0, 120.0, 0.0, 0.20);

    std::vector<double> Zs = {5.0, -1.0};
    std::vector<double> ratePath(3, 0.0);
    std::vector<double> path = barrierOpt.generatePath(1.0, 2, makeRowVector(Zs), ratePath);

    BOOST_CHECK_SMALL(path.back(), 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(CouponBond_Tests)

BOOST_AUTO_TEST_CASE(PricingAtPar) {

    CouponBond bond("Bond_Par", 1000.0, 0.05, 2.0, 0.05, 0.0);

    BOOST_CHECK_CLOSE(bond.getInitialPrice(), 1000.0, 1e-7);
}

BOOST_AUTO_TEST_CASE(SimulatedYieldPullToPar) {

    CouponBond bond("Bond_Pull", 1000.0, 0.05, 2.0, 0.05, 0.0);

    std::vector<double> Zs = {0.0};
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = bond.generatePath(1.0, 1, makeRowVector(Zs), ratePath);

    BOOST_CHECK_EQUAL(path.size(), 2);
    BOOST_CHECK_CLOSE(path.back(), 1000.0, 1e-7);
}

BOOST_AUTO_TEST_CASE(PriceDropsWhenYieldRises) {

    CouponBond bond("Bond_Volatile", 1000.0, 0.05, 5.0, 0.05, 0.10);

    std::vector<double> Zs = {2.0};
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = bond.generatePath(1.0, 1, makeRowVector(Zs), ratePath);

    BOOST_CHECK(path.back() < 1000.0);
}

BOOST_AUTO_TEST_SUITE_END()