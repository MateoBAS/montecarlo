#define BOOST_TEST_MODULE EngineAndRiskTests

#include <boost/test/unit_test.hpp>
#include <memory>
#include <vector>
#include <stdexcept>
#include <Eigen/Dense>

#include "MonteCarloEngine.h"
#include "Portfolio/Portfolio.h"
#include "Metrics/RiskCalculator.h"
#include "Asset/Asset.h"
#include "InterestRate/InterestRateModel.h"

class DummyAsset : public Asset {
public:

    DummyAsset(double p) : Asset("Dummy_Asset", p) {}

    double simulateFinalValue(double totalTime, int numSteps,
                              const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                              const std::vector<double>& ratePath) const override {
        (void)totalTime;
        (void)numSteps;
        (void)ratePath;

        double currentPrice = getInitialPrice();
        for (Eigen::Index i = 0; i < z_shocks.size(); ++i) {
            currentPrice += z_shocks[i];
        }
        return currentPrice;
    }

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                     const std::vector<double>& ratePath) const override {
        return {getInitialPrice(), simulateFinalValue(totalTime, numSteps, z_shocks, ratePath)};
    }
};
BOOST_AUTO_TEST_SUITE(Engine_And_Risk_Tests)

struct SimulacionFixture {
    Portfolio cartera;
    SimConfig config;

    SimulacionFixture() {

        auto activoA = std::make_shared<DummyAsset>(100.0);
        auto activoB = std::make_shared<DummyAsset>(50.0);
        cartera.addPosition(activoA, 10.0);
        cartera.addPosition(activoB, 20.0);

        config.totalSims = 5000;
        config.totalTime = 1.0;
        config.numSteps = 5;

        Eigen::MatrixXd corr(2, 2);
        corr << 1.0, 0.5,
                0.5, 1.0;
        config.corrMatrix = corr;
    }
};

BOOST_FIXTURE_TEST_CASE(EngineRunsSuccessfully, SimulacionFixture) {

    BOOST_CHECK_NO_THROW({
        RiskCalculator report = MonteCarloEngine::run(cartera, config);
    });
}

BOOST_FIXTURE_TEST_CASE(MathematicalCoherence, SimulacionFixture) {

    RiskCalculator report = MonteCarloEngine::run(cartera, config);

    double var95 = report.calculateVaR(0.95);
    double var99 = report.calculateVaR(0.99);
    double es99 = report.calculateES(0.99);

    BOOST_CHECK_LE(var99, var95);

    BOOST_CHECK_LE(es99, var99);
}

BOOST_FIXTURE_TEST_CASE(ThrowsOnInvalidConfidenceLevels, SimulacionFixture) {
    RiskCalculator report = MonteCarloEngine::run(cartera, config);

    BOOST_CHECK_THROW(report.calculateVaR(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(1.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(-0.05), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateVaR(1.05), std::invalid_argument);

    BOOST_CHECK_THROW(report.calculateES(0.0), std::invalid_argument);
    BOOST_CHECK_THROW(report.calculateES(1.0), std::invalid_argument);
}

class DummyRateModel : public InterestRateModel {
public:
    std::vector<double> simulateShortRatePath(double totalTime, int numSteps,
                                              const std::vector<double>& Zs) const override {
        (void)totalTime;
        (void)Zs;
        return std::vector<double>(static_cast<size_t>(numSteps) + 1, 0.01);
    }

    double discountFactorFromPath(const std::vector<double>& ratePath, double dt) const override {
        if (ratePath.empty()) {
            return 1.0;
        }
        return std::exp(-0.01 * dt * static_cast<double>(ratePath.size() - 1));
    }

    double zeroCouponBondPrice(double t, double T, double shortRateAtT) const override {
        (void)shortRateAtT;
        return std::exp(-0.01 * (T - t));
    }

    double initialShortRate() const override {
        return 0.01;
    }
};

BOOST_FIXTURE_TEST_CASE(ThrowsOnInvalidCorrMatrixWithRateModel, SimulacionFixture) {
    SimConfig badConfig = config;
    badConfig.rateModel = std::make_shared<DummyRateModel>();

    Eigen::MatrixXd corr(2, 2);
    corr << 1.0, 0.5,
            0.5, 1.0;
    badConfig.corrMatrix = corr;

    BOOST_CHECK_THROW(MonteCarloEngine::run(cartera, badConfig), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()