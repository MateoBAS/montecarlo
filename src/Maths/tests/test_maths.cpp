// Descomenta la siguiente línea si compilas este test como un ejecutable independiente
#define BOOST_TEST_MODULE MathUtilsTests
#include <boost/test/unit_test.hpp>
#include <cmath>

// Incluye tu cabecera real
#include "MathUtils.h"

BOOST_AUTO_TEST_SUITE(MathUtils_Tests)

// ==========================================
// TEST 1: Excepciones por fuera de rango (p <= 0 o p >= 1)
// ==========================================
BOOST_AUTO_TEST_CASE(ThrowsOnOutOfBoundsProbabilities) {
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(-0.5), std::invalid_argument);
    BOOST_CHECK_THROW(MathUtils::inverseNormalCDF(1.5), std::invalid_argument);
}

// ==========================================
// TEST 2: Caso central (p = 0.5 -> Z = 0)
// ==========================================
BOOST_AUTO_TEST_CASE(CenterValue) {
    // Para comparar con 0 exacto en coma flotante, se recomienda BOOST_CHECK_SMALL
    double z = MathUtils::inverseNormalCDF(0.5);
    BOOST_CHECK_SMALL(z, 1e-9); 
}

// ==========================================
// TEST 3: Valores conocidos de la CDF Normal (Aproximaciones a Z-scores enteros)
// ==========================================
BOOST_AUTO_TEST_CASE(KnownZScores) {
    // Usamos tolerancias pequeñas (porcentaje para BOOST_CHECK_CLOSE)
    const double tolerance = 1e-5;

    // Aproximadamente 1 desviación estándar
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.841344746), 1.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.158655254), -1.0, tolerance);

    // Aproximadamente 2 desviaciones estándar (Testea las colas: p < 0.02425 y p > 0.97575)
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.977249868), 2.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.022750132), -2.0, tolerance);

    // Aproximadamente 3 desviaciones estándar extremas
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.998650102), 3.0, tolerance);
    BOOST_CHECK_CLOSE(MathUtils::inverseNormalCDF(0.001349898), -3.0, tolerance);
}

// ==========================================
// TEST 4: Simetría de la distribución Normal
// ==========================================
BOOST_AUTO_TEST_CASE(DistributionSymmetry) {
    // En una normal estándar, la inversa de p debe ser el negativo de la inversa de (1-p)
    std::vector<double> testProbs = {0.01, 0.1, 0.25, 0.4, 0.6, 0.75, 0.9, 0.99};

    for (double p : testProbs) {
        double z1 = MathUtils::inverseNormalCDF(p);
        double z2 = MathUtils::inverseNormalCDF(1.0 - p);
        
        // Comprobamos que Z1 == -Z2. 
        // Usamos una comparación de valor absoluto cercana a 0
        BOOST_CHECK_SMALL(z1 + z2, 1e-9);
    }
}

BOOST_AUTO_TEST_SUITE_END()