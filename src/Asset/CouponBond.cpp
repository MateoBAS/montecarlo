#include "CouponBond.h"
#include "InterestRate/InterestRateModel.h"
#include <cmath>
#include <utility>
#include <stdexcept>

CouponBond::CouponBond(std::string name, double faceValue, double couponRate,
                       double maturity, double currentYTM, double yieldVol)
    : Asset(name, 0.0), faceValue(faceValue), couponRate(couponRate),
      maturity(maturity), currentYTM(currentYTM), yieldVol(yieldVol), rateModel(nullptr) {
    this->initialPrice = calculatePrice(currentYTM, maturity);
}

CouponBond::CouponBond(std::string name, double faceValue, double couponRate,
                       double maturity, std::shared_ptr<const InterestRateModel> rateModel)
    : Asset(name, 0.0), faceValue(faceValue), couponRate(couponRate),
      maturity(maturity), currentYTM(0.0), yieldVol(0.0), rateModel(std::move(rateModel)) {
    if (this->rateModel) {
        this->initialPrice = calculatePriceFromModel(0.0, this->rateModel->initialShortRate());
    } else {
        this->initialPrice = 0.0;
    }
}

double CouponBond::calculatePrice(double yield, double timeToMaturity) const {
    double price = 0.0;
    int numCoupons = static_cast<int>(std::floor(timeToMaturity));
    double fractionalTime = timeToMaturity - numCoupons;
    double couponPayment = faceValue * couponRate;

    for (int i = 1; i <= numCoupons; ++i) {
        price += couponPayment / std::pow(1.0 + yield, i + fractionalTime);
    }
    price += faceValue / std::pow(1.0 + yield, numCoupons + fractionalTime);

    return price;
}

double CouponBond::calculatePriceFromModel(double currentTime, double shortRateAtTime) const {
    if (!rateModel) {
        return calculatePrice(currentYTM, maturity - currentTime);
    }

    double timeRemaining = maturity - currentTime;
    if (timeRemaining <= 0.0) {
        return faceValue;
    }

    double price = 0.0;
    int numCoupons = static_cast<int>(std::floor(timeRemaining));
    double fractionalTime = timeRemaining - numCoupons;
    double couponPayment = faceValue * couponRate;

    for (int i = 1; i <= numCoupons; ++i) {
        double cashflowTime = currentTime + i + fractionalTime;
        double df = rateModel->zeroCouponBondPrice(currentTime, cashflowTime, shortRateAtTime);
        price += couponPayment * df;
    }

    double maturityTime = currentTime + numCoupons + fractionalTime;
    double dfPrincipal = rateModel->zeroCouponBondPrice(currentTime, maturityTime, shortRateAtTime);
    price += faceValue * dfPrincipal;

    return price;
}

double CouponBond::simulateBondPrice(double totalTime, int numSteps,
                                     const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                     const std::vector<double>& ratePath) const {
    if (z_shocks.size() != numSteps) {
        throw std::invalid_argument("z_shocks size must match numSteps.");
    }

    if (rateModel && !ratePath.empty()) {
        return calculatePriceFromModel(totalTime, ratePath.back());
    }

    double dt = totalTime / numSteps;
    double simulatedYTM = currentYTM;

    for (int i = 0; i < numSteps; ++i) {
        simulatedYTM += yieldVol * std::sqrt(dt) * z_shocks[i];
    }

    return calculatePrice(simulatedYTM, maturity - totalTime);
}

double CouponBond::simulateFinalValue(double totalTime, int numSteps,
                                      const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                      const std::vector<double>& ratePath) const {
    return simulateBondPrice(totalTime, numSteps, z_shocks, ratePath);
}

std::vector<double> CouponBond::generatePath(double totalTime, int numSteps,
                                             const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                             const std::vector<double>& ratePath) const {
    return {getInitialPrice(), simulateFinalValue(totalTime, numSteps, z_shocks, ratePath)};
}
