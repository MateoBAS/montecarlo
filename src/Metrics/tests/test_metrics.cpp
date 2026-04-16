// Descomenta la siguiente línea si compilas este test como un ejecutable independiente
#define BOOST_TEST_MODULE RiskCalculatorTests
#include <boost/test/unit_test.hpp>
#include <vector>
#include <stdexcept>

// Incluye tu cabecera real
#include "RiskCalculator.h"

// ==========================================
// SUITE: RiskCalculator Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(RiskCalculator_Tests)

BOOST_AUTO_TEST_CASE(InvalidConfidenceLevelThrows) {
    std::vector<double> pnls = {1.0, 2.0, 3.0};
    RiskCalculator riskCalc(pnls);

    // Niveles de confianza fuera del rango (0, 1) deben lanzar std::invalid_argument
    BOOST_CHECK_THROW(riskCalc.calculateVaR(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(1.05), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateVaR(-0.1), std::invalid_argument);

    BOOST_CHECK_THROW(riskCalc.calculateES(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(riskCalc.calculateES(1.0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(VaR_Calculation) {
    // 10 valores desordenados. 
    // Ordenados serán: -100, -50, -20, -10, -5, 0, 10, 20, 50, 100
    std::vector<double> pnls = {10.0, -20.0, 100.0, -100.0, -50.0, 0.0, 20.0, -5.0, 50.0, -10.0};
    RiskCalculator riskCalc(pnls);

    // Confianza del 90% (alpha = 0.10)
    // indice = trunc(10 * 0.10) = 1
    // PnL en indice 1 de la lista ordenada (-100, -50, ...) es -50.0
    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.90), -50.0, 1e-9);

    // Confianza del 80% (alpha = 0.20)
    // indice = trunc(10 * 0.20) = 2
    // PnL en indice 2 es -20.0
    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.80), -20.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(ES_Calculation) {
    // 10 valores desordenados.
    // Ordenados: -100, -50, -20, -10, -5, 0, 10, 20, 50, 100
    std::vector<double> pnls = {50.0, -100.0, -5.0, -50.0, 100.0, -20.0, -10.0, 10.0, 20.0, 0.0};
    RiskCalculator riskCalc(pnls);

    // Confianza del 80% (alpha = 0.20)
    // indice VaR = 2
    // ES es la media de los valores ANTES del índice VaR: (-100 + -50) / 2 = -75.0
    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.80), -75.0, 1e-9);

    // Confianza del 70% (alpha = 0.30)
    // indice VaR = 3
    // ES es la media de: (-100 + -50 + -20) / 3 = -170 / 3 = -56.6666666...
    double expectedES = (-100.0 - 50.0 - 20.0) / 3.0;
    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.70), expectedES, 1e-9);
}

BOOST_AUTO_TEST_CASE(ExtremeConfidenceLevels_ZeroIndex) {
    // Si pedimos una confianza tan alta que el índice es 0, tu código
    // maneja el ES de forma especial devolviendo el peor caso absoluto (índice 0).
    std::vector<double> pnls = {-500.0, -100.0, -50.0, 10.0, 50.0};
    RiskCalculator riskCalc(pnls);

    // Para 5 elementos y 99% de confianza (alpha = 0.01)
    // indice = trunc(5 * 0.01) = 0
    BOOST_CHECK_CLOSE(riskCalc.calculateVaR(0.99), -500.0, 1e-9);
    BOOST_CHECK_CLOSE(riskCalc.calculateES(0.99), -500.0, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()