#pragma once
#include "Asset.h"

class GBMAsset : public Asset {
private:
    double driftSpread;
    double volatility;

public:
    GBMAsset(std::string name, double initialPrice, double drift, double volatility);

    std::vector<double> generatePath(double totalTime, int numSteps,
                                     const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                     const std::vector<double>& ratePath) const override;
};