// Descomenta la siguiente línea si compilas este test como un ejecutable independiente
#define BOOST_TEST_MODULE PortfolioTests 
#include <boost/test/unit_test.hpp>
/*#include <memory>
#include <vector>
#include <stdexcept>
#include <cmath>

#include <Eigen/Dense>

// Incluye tus cabeceras reales
#include "Portfolio/Portfolio.h"
#include "Asset/Asset.h"
#include "Asset/GBMAsset.h" 
#include "InterestRate/InterestRateModel.h"

// ==========================================
// MOCK ASSET PARA AISLAR LOS TESTS UNITARIOS
// ==========================================
class MockAsset : public Asset {
private:
    double finalSimulatedPrice;

public:
    // Llamamos explícitamente al constructor de la clase base Asset
    MockAsset(std::string name, double initPrice, double finalPrice) 
        : Asset(std::move(name), initPrice), finalSimulatedPrice(finalPrice) {}

    double simulateFinalValue(double totalTime, int numSteps,
                              const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                              const std::vector<double>& ratePath) const override {
        (void)totalTime;
        (void)numSteps;
        (void)z_shocks;
        (void)ratePath;
        return finalSimulatedPrice;
    }

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                     const std::vector<double>& ratePath) const override {
        (void)totalTime;
        (void)z_shocks;
        (void)ratePath;
        std::vector<double> path(numSteps + 1, getInitialPrice());
        path.back() = finalSimulatedPrice;
        return path;
    }
};

// ==========================================
// SUITE: Portfolio Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(Portfolio_Tests)

BOOST_AUTO_TEST_CASE(EmptyPortfolio) {
    Portfolio port;
    BOOST_CHECK_EQUAL(port.getNumAssets(), 0);
    BOOST_CHECK_EQUAL(port.getInitialValue(), 0.0);
}

BOOST_AUTO_TEST_CASE(AddPositionsAndInitialValue) {
    Portfolio port;
    
    // Activo A: Precio inicial 100
    auto assetA = std::make_shared<MockAsset>("MockA", 100.0, 110.0);
    // Activo B: Precio inicial 50
    auto assetB = std::make_shared<MockAsset>("MockB", 50.0, 40.0);

    // Añadimos 10 unidades de A y 20 de B
    port.addPosition(assetA, 10.0);
    port.addPosition(assetB, 20.0);

    BOOST_CHECK_EQUAL(port.getNumAssets(), 2);

    // Valor inicial esperado = (100 * 10) + (50 * 20) = 1000 + 1000 = 2000
    BOOST_CHECK_CLOSE(port.getInitialValue(), 2000.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(SimulatePathPnL_ThrowsOnZMatrixMismatch) {
    Portfolio port;
    auto assetA = std::make_shared<MockAsset>("MockA", 100.0, 110.0);
    port.addPosition(assetA, 10.0); // 1 activo en cartera

    // Matriz Z con 2 filas (inconsistente con el número de activos)
    std::vector<std::vector<double>> badZMatrix = {
        {0.1, 0.2}, 
        {-0.1, -0.2}
    };

    std::vector<double> ratePath(3, 0.0);
    BOOST_CHECK_THROW(port.simulatePathPnL(1.0, 2, badZMatrix, ratePath, nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(SimulatePathPnL_CalculatesCorrectly) {
    Portfolio port;
    
    // MockA: S0 = 100 -> S_T = 110 (+10 beneficio por unidad)
    auto assetA = std::make_shared<MockAsset>("MockA", 100.0, 110.0);
    // MockB: S0 = 50 -> S_T = 40 (-10 pérdida por unidad)
    auto assetB = std::make_shared<MockAsset>("MockB", 50.0, 40.0);

    port.addPosition(assetA, 10.0); // PnL posición = 10 * 10 = +100
    port.addPosition(assetB, 5.0);  // PnL posición = 5 * (-10) = -50

    // La matriz Z debe tener 2 filas (para 2 activos). En el Mock no se usan, pero la forma debe ser válida.
    std::vector<std::vector<double>> zMatrix = {
        {0.0, 0.0},
        {0.0, 0.0}
    };
    std::vector<double> ratePath(3, 0.0);

    // PnL Esperado = 100 - 50 = +50.0
    double expectedPnL = 50.0;
    double actualPnL = port.simulatePathPnL(1.0, 2, zMatrix, ratePath, nullptr);

    BOOST_CHECK_CLOSE(actualPnL, expectedPnL, 1e-9);
}

// ==========================================
// TEST DE INTEGRACIÓN: Portfolio + GBMAsset
// ==========================================
BOOST_AUTO_TEST_CASE(Integration_PortfolioWithGBMAsset) {
    Portfolio port;
    
    // Creamos un activo real: S0=100, drift=5%, vol=0% (crecimiento determinista para poder calcular a mano)
    auto realGBM = std::make_shared<GBMAsset>("RealStock", 100.0, 0.05, 0.0);
    port.addPosition(realGBM, 10.0); // 10 acciones

    double totalTime = 1.0;
    int numSteps = 5;
    
    // Matriz Z correcta: 1 activo, 5 pasos
    std::vector<std::vector<double>> zMatrix = {
        {0.0, 0.0, 0.0, 0.0, 0.0}
    };
    std::vector<double> ratePath(numSteps + 1, 0.0);

    double initialValue = port.getInitialValue(); // 100 * 10 = 1000
    
    // Con vol = 0, el precio final real será S0 * exp(drift * T) = 100 * exp(0.05)
    double expectedFinalPrice = 100.0 * std::exp(0.05 * 1.0);
    double expectedFutureValue = expectedFinalPrice * 10.0;
    double expectedPnL = expectedFutureValue - initialValue;

    double actualPnL = port.simulatePathPnL(totalTime, numSteps, zMatrix, ratePath, nullptr);

    BOOST_CHECK_CLOSE(actualPnL, expectedPnL, 1e-7);
}

// ==========================================
// TEST: PnL Discounted With Rate Model
// ==========================================
class FlatRateModel : public InterestRateModel {
public:
    explicit FlatRateModel(double r) : rate(r) {}

    std::vector<double> simulateShortRatePath(double totalTime, int numSteps,
                                              const std::vector<double>& Zs) const override {
        (void)totalTime;
        (void)Zs;
        return std::vector<double>(static_cast<size_t>(numSteps) + 1, rate);
    }

    double discountFactorFromPath(const std::vector<double>& ratePath, double dt) const override {
        if (ratePath.empty()) {
            return 1.0;
        }
        return std::exp(-rate * dt * static_cast<double>(ratePath.size() - 1));
    }

    double zeroCouponBondPrice(double t, double T, double shortRateAtT) const override {
        (void)shortRateAtT;
        return std::exp(-rate * (T - t));
    }

    double initialShortRate() const override {
        return rate;
    }

private:
    double rate;
};

BOOST_AUTO_TEST_CASE(DiscountsFutureValue) {
    Portfolio port;
    auto assetA = std::make_shared<MockAsset>("MockA", 100.0, 110.0);
    port.addPosition(assetA, 1.0);

    std::vector<std::vector<double>> zMatrix = {{0.0}};
    std::vector<double> ratePath = {0.05, 0.05};
    FlatRateModel model(0.05);

    double pnl = port.simulatePathPnL(1.0, 1, zMatrix, ratePath, &model);

    double discountedFuture = 110.0 * std::exp(-0.05 * 1.0);
    double expectedPnL = discountedFuture - 100.0;

    BOOST_CHECK_CLOSE(pnl, expectedPnL, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()*/