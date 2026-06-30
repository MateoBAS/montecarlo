#define BOOST_TEST_MODULE MonteCarloGeneratorsTest
#include <boost/test/included/unit_test.hpp>
#include <vector>
#include <Eigen/Dense>

#include "RandomGenerator.h"

BOOST_AUTO_TEST_SUITE(MersenneTwister_Tests)

BOOST_AUTO_TEST_CASE(SeedReproducibility) {

    MersenneTwisterGen mt1(42);
    MersenneTwisterGen mt2(42);

    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK_CLOSE(mt1.getStandardNormal(), mt2.getStandardNormal(), 1e-9);
    }
}

BOOST_AUTO_TEST_CASE(VectorGeneration) {
    MersenneTwisterGen mt(123);
    std::vector<double> vec(10);
    mt.generateStandardNormals(vec);

    BOOST_CHECK_EQUAL(vec.size(), 10);

    BOOST_CHECK(vec[0] != vec[1]);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Sobol_Tests)

BOOST_AUTO_TEST_CASE(BasicGeneration) {
    BoostSobolGenerator sobol(1);

    double val = sobol.getStandardNormal();

    BOOST_CHECK(!std::isnan(val));
    BOOST_CHECK(!std::isinf(val));
}

BOOST_AUTO_TEST_CASE(SkipFunctionality) {
    BoostSobolGenerator sobol1(1);
    BoostSobolGenerator sobol2(1);

    for (int i = 0; i < 5; ++i) sobol1.getStandardNormal();

    sobol2.skip(5);

    BOOST_CHECK_CLOSE(sobol1.getStandardNormal(), sobol2.getStandardNormal(), 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()
