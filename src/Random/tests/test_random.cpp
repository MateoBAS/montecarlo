#define BOOST_TEST_MODULE MonteCarloGeneratorsTest
#include <boost/test/included/unit_test.hpp>
#include <vector>
#include <Eigen/Dense>

#include "RandomGenerator.h"

BOOST_AUTO_TEST_SUITE(MersenneTwister_Tests)

BOOST_AUTO_TEST_CASE(SeedReproducibility) {
    // Comprobamos que la misma semilla genere los mismos números
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
    // Verificamos que los números no sean todos idénticos (algo extremadamente raro en MT)
    BOOST_CHECK(vec[0] != vec[1]);
}

BOOST_AUTO_TEST_SUITE_END()

// ==========================================
// SUITE 2: BoostSobolGenerator Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(Sobol_Tests)

// Nota: Este test asume que MathUtils::inverseNormalCDF está implementado correctamente.
BOOST_AUTO_TEST_CASE(BasicGeneration) {
    BoostSobolGenerator sobol(1); // 1 dimensión
    
    double val = sobol.getStandardNormal();
    // Solo comprobamos que se genera un número válido (no es NaN ni infinito)
    BOOST_CHECK(!std::isnan(val));
    BOOST_CHECK(!std::isinf(val));
}

BOOST_AUTO_TEST_CASE(SkipFunctionality) {
    BoostSobolGenerator sobol1(1);
    BoostSobolGenerator sobol2(1);

    // Generamos 5 números con el primero
    for (int i = 0; i < 5; ++i) sobol1.getStandardNormal();

    // Saltamos 5 pasos en el segundo
    sobol2.skip(5);

    // El número 6 debería coincidir
    BOOST_CHECK_CLOSE(sobol1.getStandardNormal(), sobol2.getStandardNormal(), 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()
