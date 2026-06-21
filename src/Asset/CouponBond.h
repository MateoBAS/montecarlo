#pragma once
#include "Asset.h"
#include <vector>
#include <memory>

class InterestRateModel;

class CouponBond : public Asset {
private:
    double faceValue;
    double couponRate;
    double maturity;
    double currentYTM;
    double yieldVol;
    std::shared_ptr<const InterestRateModel> rateModel;

    double calculatePrice(double yield, double timeToMaturity) const;
    double calculatePriceFromModel(double currentTime, double shortRateAtTime) const;

    double simulateBondPrice(double totalTime, int numSteps,
                             const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                             const std::vector<double>& ratePath) const;

public:
    CouponBond(std::string name, double faceValue, double couponRate,
               double maturity, double currentYTM, double yieldVol);

    CouponBond(std::string name, double faceValue, double couponRate,
               double maturity, std::shared_ptr<const InterestRateModel> rateModel);

    double simulateFinalValue(double totalTime, int numSteps,
                              const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                              const std::vector<double>& ratePath) const override;

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                     const std::vector<double>& ratePath) const override;
};
