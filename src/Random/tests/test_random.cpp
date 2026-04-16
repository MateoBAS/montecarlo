#define BOOST_TEST_MODULE MonteCarloGeneratorsTest
#include <boost/test/included/unit_test.hpp>
#include <vector>
#include <Eigen/Dense>

#include "RandomGenerator.h"
#include "CorrelatedGenerator.h"

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
// SUITE 2: AntitheticGenerator Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(Antithetic_Tests)

BOOST_AUTO_TEST_CASE(AntitheticPairs) {
    MersenneTwisterGen mt(42);
    AntitheticGenerator antiGen(&mt);

    // El generador debe devolver Z, luego -Z
    double z1 = antiGen.getStandardNormal();
    double z2 = antiGen.getStandardNormal();
    BOOST_CHECK_CLOSE(z1, -z2, 1e-9); // Z1 debe ser igual a -Z2

    // El siguiente par debe ser diferente al primero, pero opuesto entre sí
    double z3 = antiGen.getStandardNormal();
    double z4 = antiGen.getStandardNormal();
    BOOST_CHECK_CLOSE(z3, -z4, 1e-9);
    
    BOOST_CHECK(z1 != z3); 
}

BOOST_AUTO_TEST_CASE(AntitheticVector) {
    MersenneTwisterGen mt(100);
    AntitheticGenerator antiGen(&mt);
    
    std::vector<double> vec(4);
    antiGen.generateStandardNormals(vec);

    // Revisamos que los pares contiguos se anulen (z, -z, z2, -z2)
    BOOST_CHECK_CLOSE(vec[0], -vec[1], 1e-9);
    BOOST_CHECK_CLOSE(vec[2], -vec[3], 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()

// ==========================================
// SUITE 3: BoostSobolGenerator Tests
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

// ==========================================
// SUITE 4: CorrelatedGenerator Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(Correlated_Tests)

BOOST_AUTO_TEST_CASE(InvalidMatrixThrows) {
    MersenneTwisterGen mt(42);
    
    // Matriz no cuadrada
    Eigen::MatrixXd nonSquare(2, 3);
    BOOST_CHECK_THROW(CorrelatedGenerator(&mt, nonSquare), std::invalid_argument);

    // Matriz no definida positiva (ej. correlación mayor a 1 o estructura inválida)
    Eigen::MatrixXd nonPD(2, 2);
    nonPD << 1.0, 2.0, 
             2.0, 1.0; 
    BOOST_CHECK_THROW(CorrelatedGenerator(&mt, nonPD), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ValidCorrelationGeneration) {
    MersenneTwisterGen mt(42);
    
    Eigen::MatrixXd corrMat(3, 3);
    corrMat << 1.0, 0.5, 0.2,
               0.5, 1.0, 0.3,
               0.2, 0.3, 1.0;

    CorrelatedGenerator corrGen(&mt, corrMat);
    std::vector<double> results = corrGen.getCorrelatedNormals();

    BOOST_CHECK_EQUAL(results.size(), 3);
    
    // Verificamos que los números generados no sean NaN
    for(double z : results) {
        BOOST_CHECK(!std::isnan(z));
    }
}

BOOST_AUTO_TEST_SUITE_END()