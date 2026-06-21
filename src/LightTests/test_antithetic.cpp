#define BOOST_TEST_MODULE AntitheticEngineTests
#include <boost/test/unit_test.hpp>

#include <cmath>
#include <memory>

#include <Eigen/Dense>

#include "Asset/GBMAsset.h"
#include "MontecarloEngine/MontecarloEngine.h"
#include "Portfolio/Portfolio.h"

BOOST_AUTO_TEST_SUITE(AntitheticEngine_Tests)

BOOST_AUTO_TEST_CASE(VaR_IsCloseToMersenneTwister) {
    Portfolio portfolio;
    auto stock = std::make_shared<GBMAsset>("Equity", 100.0, 0.02, 0.20);
    portfolio.addPosition(stock, 50.0);

    SimConfig mtConfig;
    mtConfig.totalSims = 4000;
    mtConfig.totalTime = 1.0;
    mtConfig.numSteps = 25;
    mtConfig.numCores = 1;
    mtConfig.rngType = RNGType::MersenneTwister;
    mtConfig.corrMatrix = Eigen::MatrixXd::Identity(1, 1);

    SimConfig antiConfig = mtConfig;
    antiConfig.rngType = RNGType::Antithetic;

    const double varMt = MonteCarloEngine::run(portfolio, mtConfig).calculateVaR(0.99);
    const double varAnti = MonteCarloEngine::run(portfolio, antiConfig).calculateVaR(0.99);

    BOOST_CHECK_LT(varAnti, 0.0);
    BOOST_CHECK_LT(varMt, 0.0);
    BOOST_CHECK_CLOSE(varAnti, varMt, 20.0);
}

BOOST_AUTO_TEST_SUITE_END()
