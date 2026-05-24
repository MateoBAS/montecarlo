#define BOOST_TEST_MODULE InterestRateTests
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <vector>

#include "InterestRate/YieldCurve.h"
#include "InterestRate/HullWhiteModel.h"

BOOST_AUTO_TEST_SUITE(YieldCurve_Tests)

BOOST_AUTO_TEST_CASE(InterpolatesZeroRate) {
    YieldCurve curve({1.0, 2.0, 3.0}, {0.02, 0.03, 0.04});

    double r = curve.zeroRate(1.5);
    double expected = 0.02 + 0.5 * (0.03 - 0.02);
    BOOST_CHECK_CLOSE(r, expected, 1e-9);
}

BOOST_AUTO_TEST_CASE(DiscountFactorAtZero) {
    YieldCurve curve({1.0}, {0.05});

    BOOST_CHECK_CLOSE(curve.discountFactor(0.0), 1.0, 1e-12);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(HullWhite_Tests)

BOOST_AUTO_TEST_CASE(PathLengthAndInitialRate) {
    YieldCurve curve({1.0, 2.0, 3.0}, {0.02, 0.02, 0.02});
    HullWhiteModel hw(0.1, 0.01, curve);

    int steps = 4;
    std::vector<double> Zs(steps, 0.0);
    std::vector<double> path = hw.simulateShortRatePath(1.0, steps, Zs);

    BOOST_CHECK_EQUAL(path.size(), static_cast<size_t>(steps + 1));
    BOOST_CHECK_CLOSE(path.front(), hw.initialShortRate(), 1e-12);
}

BOOST_AUTO_TEST_CASE(DiscountFactorMatchesIntegral) {
    YieldCurve curve({1.0}, {0.03});
    HullWhiteModel hw(0.1, 0.0, curve, 0.03);

    std::vector<double> ratePath = {0.03, 0.03, 0.03};
    double dt = 0.5;
    double df = hw.discountFactorFromPath(ratePath, dt);

    double expected = std::exp(-0.03 * 1.0);
    BOOST_CHECK_CLOSE(df, expected, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()
