// Descomenta la siguiente línea si compilas este test como un ejecutable independiente
#define BOOST_TEST_MODULE AssetTests
#include <boost/test/unit_test.hpp>
#include <vector>
#include <cmath>

#include <Eigen/Dense>

// Incluye tus cabeceras reales
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

// ==========================================
// SUITE 1: GBMAsset Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(GBMAsset_Tests)

BOOST_AUTO_TEST_CASE(DeterministicGrowth_ZeroVol) {
    // S0 = 100, drift = 5%, vol = 0
    GBMAsset stock("StockA", 100.0, 0.05, 0.0);
    
    // Al no haber volatilidad, los Zs no importan
    std::vector<double> Zs = {0.0, 0.0, 0.0}; 
    double totalTime = 1.0;
    int numSteps = 3;
    std::vector<double> ratePath(numSteps + 1, 0.0);
    
    std::vector<double> path = stock.generatePath(totalTime, numSteps, makeRowVector(Zs), ratePath);
    
    BOOST_CHECK_EQUAL(path.size(), 4); // S0 + 3 pasos
    BOOST_CHECK_EQUAL(path[0], 100.0);
    
    // Con vol = 0, el precio final debe ser S0 * exp(drift * T)
    double expectedFinalPrice = 100.0 * std::exp(0.05 * 1.0);
    BOOST_CHECK_CLOSE(path.back(), expectedFinalPrice, 1e-7);
}

BOOST_AUTO_TEST_CASE(StochasticMovement) {
    // S0 = 100, drift = 0, vol = 20%
    GBMAsset stock("StockB", 100.0, 0.0, 0.20);
    
    // Forzamos Z = 1.0 en un solo paso
    std::vector<double> Zs = {1.0}; 
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = stock.generatePath(1.0, 1, makeRowVector(Zs), ratePath);
    
    // Drift term = (0 - 0.5 * 0.2^2) * 1 = -0.02
    // Vol term = 0.2 * sqrt(1) * 1 = 0.2
    // Exponente total = 0.18
    double expectedPrice = 100.0 * std::exp(0.18);
    BOOST_CHECK_CLOSE(path.back(), expectedPrice, 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()

// ==========================================
// SUITE 2: EuropeanOption Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(EuropeanOption_Tests)

BOOST_AUTO_TEST_CASE(CallOption_InTheMoney) {
    // Prima=5, S0=100, K=100, drift=10%, vol=0, tipo=Call
    EuropeanOption call("Call_ITM", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Call);
    std::vector<double> Zs = {0.0};
    std::vector<double> ratePath(2, 0.0);
    
    std::vector<double> path = call.generatePath(1.0, 1, makeRowVector(Zs), ratePath);
    
    // S_T = 100 * exp(0.10) = 110.517
    // Payoff Call = Max(S_T - K, 0) = 10.517
    double expectedPayoff = 100.0 * std::exp(0.10) - 100.0;
    
    BOOST_CHECK_EQUAL(path[0], 5.0); // El valor inicial es la prima
    BOOST_CHECK_CLOSE(path.back(), expectedPayoff, 1e-7);
}

BOOST_AUTO_TEST_CASE(PutOption_OutOfTheMoney) {
    // Prima=5, S0=100, K=100, drift=10%, vol=0, tipo=Put
    EuropeanOption put("Put_OTM", 5.0, 100.0, 100.0, 0.10, 0.0, OptionType::Put);
    std::vector<double> Zs = {0.0};
    std::vector<double> ratePath(2, 0.0);
    
    std::vector<double> path = put.generatePath(1.0, 1, makeRowVector(Zs), ratePath);
    
    // S_T = 110.517. Como K=100 y es Put, OTM -> Payoff = 0.0
    BOOST_CHECK_SMALL(path.back(), 1e-9); 
}

BOOST_AUTO_TEST_SUITE_END()

// ==========================================
// SUITE 3: BarrierOption Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(BarrierOption_Tests)

BOOST_AUTO_TEST_CASE(UpAndOut_NoKnockOut) {
    // Prima=5, S0=100, K=100, Barrera=120
    BarrierOption barrierOpt("Barrier_Safe", 5.0, 100.0, 100.0, 120.0, 0.0, 0.20);
    
    // 2 pasos. Zs forzados para subir un poco pero sin tocar 120
    std::vector<double> Zs = {0.5, 0.5}; 
    std::vector<double> ratePath(3, 0.0);
    std::vector<double> path = barrierOpt.generatePath(1.0, 2, makeRowVector(Zs), ratePath);
    
    // Como no supera 120, debe tener un payoff positivo (es una Call por defecto en tu código)
    BOOST_CHECK(path.back() > 0.0);
}

BOOST_AUTO_TEST_CASE(UpAndOut_KnocksOut) {
    // Prima=5, S0=100, K=100, Barrera=120
    BarrierOption barrierOpt("Barrier_Dead", 5.0, 100.0, 100.0, 120.0, 0.0, 0.20);
    
    // El primer Z=5.0 forzará un salto enorme en el primer dt, superando 120 seguro
    std::vector<double> Zs = {5.0, -1.0}; 
    std::vector<double> ratePath(3, 0.0);
    std::vector<double> path = barrierOpt.generatePath(1.0, 2, makeRowVector(Zs), ratePath);
    
    // Se ha activado la barrera, el payoff final DEBE ser 0.0
    BOOST_CHECK_SMALL(path.back(), 1e-9);
}

BOOST_AUTO_TEST_SUITE_END()

// ==========================================
// SUITE 4: CouponBond Tests
// ==========================================
BOOST_AUTO_TEST_SUITE(CouponBond_Tests)

BOOST_AUTO_TEST_CASE(PricingAtPar) {
    // Nominal=1000, Cupon=5%, Vencimiento=2 años, TIR=5%, Vol=0
    // Cuando el Cupón == TIR, el bono cotiza a la par (Precio = Nominal)
    CouponBond bond("Bond_Par", 1000.0, 0.05, 2.0, 0.05, 0.0);
    
    BOOST_CHECK_CLOSE(bond.getInitialPrice(), 1000.0, 1e-7);
}

BOOST_AUTO_TEST_CASE(SimulatedYieldPullToPar) {
    // Nominal=1000, Cupon=5%, Vencimiento=2 años, TIR=5%, Vol=0
    CouponBond bond("Bond_Pull", 1000.0, 0.05, 2.0, 0.05, 0.0);
    
    // Simulamos 1 año entero. Los tipos no cambian (Vol=0, Z=0)
    std::vector<double> Zs = {0.0}; 
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = bond.generatePath(1.0, 1, makeRowVector(Zs), ratePath);
    
    // Al quedar 1 año, y seguir la TIR al 5%, el precio sigue siendo 1000
    // NOTA: Tu código actual devuelve un path de tamaño 2 para bonos [PrecioInicial, PrecioFinal]
    BOOST_CHECK_EQUAL(path.size(), 2); 
    BOOST_CHECK_CLOSE(path.back(), 1000.0, 1e-7);
}

BOOST_AUTO_TEST_CASE(PriceDropsWhenYieldRises) {
    // Volatilidad de tipos = 10%
    CouponBond bond("Bond_Volatile", 1000.0, 0.05, 5.0, 0.05, 0.10);
    
    // Forzamos un Z positivo para que la TIR suba artificialmente
    std::vector<double> Zs = {2.0}; 
    std::vector<double> ratePath(2, 0.0);
    std::vector<double> path = bond.generatePath(1.0, 1, makeRowVector(Zs), ratePath);
    
    // Si la TIR sube, el precio del bono DEBE caer por debajo de 1000
    BOOST_CHECK(path.back() < 1000.0);
}

BOOST_AUTO_TEST_SUITE_END()