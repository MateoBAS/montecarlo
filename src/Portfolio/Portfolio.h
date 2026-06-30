
#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <Eigen/Dense>
#include "../Asset/Asset.h"
#include "../InterestRate/InterestRateModel.h"

class InterestRateModel;

struct Position {
    std::shared_ptr<Asset> asset;
    double quantity;
};

class Portfolio {
private:
    std::vector<Position> positions;
    double initialValue = 0.0;

public:
    void addPosition(std::shared_ptr<Asset> asset, double quantity);

    double getInitialValue() const { return initialValue; }
    size_t getNumAssets() const;

    template<typename Derived>
    double simulatePathPnL(double totalTime, int numSteps,
                           const Eigen::MatrixBase<Derived>& Z_matrix,
                           const std::vector<double>& ratePath,
                           const InterestRateModel* rateModel) const {

        if (Z_matrix.rows() != static_cast<int>(positions.size())) {
            throw std::invalid_argument("El numero de filas en Z_matrix debe coincidir con los activos del portfolio.");
        }
        if (!ratePath.empty() && ratePath.size() != static_cast<size_t>(numSteps + 1)) {
            throw std::invalid_argument("El ratePath debe tener numSteps + 1 elementos.");
        }

        double futureValue = 0.0;
        for (size_t i = 0; i < positions.size(); ++i) {
            double finalPrice = positions[i].asset->simulateFinalValue(
                totalTime, numSteps, Z_matrix.row(i), ratePath);
            futureValue += finalPrice * positions[i].quantity;
        }

        double discountFactor = 1.0;
        if (rateModel && !ratePath.empty()) {
            discountFactor = rateModel->discountFactorFromPath(ratePath, totalTime / numSteps);
        }

        double discountedValue = futureValue * discountFactor;

        return discountedValue - initialValue;
    }
};