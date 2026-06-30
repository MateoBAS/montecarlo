
#define BOOST_TEST_MODULE MathUtilsTests
#include <boost/test/unit_test.hpp>
#include <cmath>

#include "MathUtils.h"

BOOST_AUTO_TEST_SUITE(MathUtils_Tests)

BOOST_AUTO_TEST_CASE(ThrowsOnOutOfBoundsProbabilities) {
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(-0.5), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(1.5), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(CenterValue) {

    double z = MathUtils::inverseNormalCDF(0.5);
    BOOST_CHECK_SMALL(z, 1e-9);
}

BOOST_AUTO_TEST_CASE(KnownZScores) {

    const double tolerance = 1e-5;

    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.841344746), 1.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.158655254), -1.0, tolerance);

    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.977249868), 2.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.022750132), -2.0, tolerance);

    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.998650102), 3.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.001349898), -3.0, tolerance);
}

BOOST_AUTO_TEST_CASE(DistributionSymmetry) {

    std::vector<double> testProbs = {0.01, 0.1, 0.25, 0.4, 0.6, 0.75, 0.9, 0.99};

    for (double p : testProbs) {
        double z1 = MathUtils::inverseNormalCDF(p);
        double z2 = MathUtils::inverseNormalCDF(1.0 - p);

        BOOST_CHECK_SMALL(z1 + z2, 1e-9);
    }
}

BOOST_AUTO_TEST_SUITE_END()