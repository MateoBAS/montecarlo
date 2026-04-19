#define BOOST_TEST_MODULE EngineAndRiskTests 

#include <boost/test/unit_test.hpp>
#include <memory>
#include <vector>
#include <stdexcept>
#include <Eigen/Dense>

// Incluye tus cabeceras reales
#include "MonteCarloEngine.h"
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"
#include "Asset/Asset.h"

class DummyAsset : public Asset {
public:
    // Llamamos al constructor de la clase base pasándole un nombre inventado y el precio
    DummyAsset(double p) : Asset("Dummy_Asset", p) {}
    
    // OJO: No hacemos override de getInitialPrice() porque no es virtual en Asset.h
    // La clase base Asset ya se encarga de devolvernos el precio inicial correctamente.

    // Sobrescribimos la función que SÍ es virtual pura (= 0), respetando su 'const' al final
    std::vector<double> generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const override {
        std::vector<double> path;
        
        // Podemos usar getInitialPrice() porque lo heredamos de la clase padre
        double currentPrice = getInitialPrice(); 
        
        for(double z : Zs) {
            currentPrice += z; 
            path.push_back(currentPrice);
        }
        return path;
    }
};
BOOST_AUTO_TEST_SUITE(Engine_And_Risk_Tests)

// ==========================================
// FIXTURE: Entorno de prueba reutilizable
// ==========================================
struct SimulacionFixture {
    Portfolio cartera;
    SimConfig config;

    SimulacionFixture() {
        // 1. Configuramos una cartera sencilla con activos dummy
        auto activoA = std::make_shared<DummyAsset>(100.0);
        auto activoB = std::make_shared<DummyAsset>(50.0);
        cartera.addPosition(activoA, 10.0); // 1000 EUR
        cartera.addPosition(activoB, 20.0); // 1000 EUR

        // 2. Configuramos los parámetros del motor
        config.totalSims = 5000; // Suficiente para tests unitarios rápidos
        config.totalTime = 1.0;
        config.numSteps = 5;
        
        // Matriz de correlación simple 2x2
        Eigen::MatrixXd corr(2, 2);
        corr << 1.0, 0.5,
                0.5, 1.0;
        config.corrMatrix = corr;
    }
};

// ==========================================
// TEST 1: Ejecución del motor sin errores
// ==========================================
BOOST_FIXTURE_TEST_CASE(EngineRunsSuccessfully, SimulacionFixture) {
    // Comprobamos que el motor ejecuta y devuelve el calculador sin lanzar excepciones
    BOOST_CHECK_NO_THROW({
        RiskCalculator report = MonteCarloEngine::run(cartera, config);
    });
}

// ==========================================
// TEST 2: Coherencia Matemática de VaR y ES
// ==========================================
BOOST_FIXTURE_TEST_CASE(MathematicalCoherence, SimulacionFixture) {
    // Generamos el reporte UNA SOLA VEZ
    RiskCalculator report = MonteCarloEngine::run(cartera, config);

    double var95 = report.calculateVaR(0.95);
    double var99 = report.calculateVaR(0.99);
    double es99 = report.calculateES(0.99);

    // En términos de PnL (donde las pérdidas son números negativos):
    // El 1% de peores casos (VaR 99%) debe ser un número MENOR o igual 
    // (es decir, una pérdida peor) que el 5% de peores casos (VaR 95%).
    BOOST_CHECK_LE(var99, var95);

    // El Expected Shortfall (promedio de la cola del 1%) debe ser MENOR o igual
    // (pérdida aún más extrema) que el umbral exacto del VaR al 99%.
    BOOST_CHECK_LE(es99, var99);
}

// ==========================================
// TEST 3: Excepciones por niveles de confianza inválidos
// ==========================================
BOOST_FIXTURE_TEST_CASE(ThrowsOnInvalidConfidenceLevels, SimulacionFixture) {
    RiskCalculator report = MonteCarloEngine::run(cartera, config);

    // Probabilidades fuera del rango (0, 1) en VaR
    BOOST_CHECK_THROW(report.calculateVaR(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(-0.05), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(1.05), std::invalid_argument);

    // Probabilidades fuera del rango en ES
    BOOST_CHECK_THROW(report.calculateES(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateES(1.0), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()