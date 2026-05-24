#include "Portfolio.h"
#include "InterestRate/InterestRateModel.h"
#include <stdexcept>

void Portfolio::addPosition(std::shared_ptr<Asset> asset, double quantity) {
    positions.push_back({asset, quantity});
}

double Portfolio::getInitialValue() const {
    double totalValue = 0.0;
    for (const auto& pos : positions) {
        totalValue += pos.asset->getInitialPrice() * pos.quantity;
    }
    return totalValue;
}

size_t Portfolio::getNumAssets() const {
    return positions.size();
}

double Portfolio::simulatePathPnL(double totalTime, int numSteps,
                                  const std::vector<std::vector<double>>& Z_matrix,
                                  const std::vector<double>& ratePath,
                                  const InterestRateModel* rateModel) const {
    if (Z_matrix.size() != positions.size()) {
        throw std::invalid_argument("El numero de filas en Z_matrix debe coincidir con los activos del portfolio.");
    }
    if (!ratePath.empty() && ratePath.size() != static_cast<size_t>(numSteps + 1)) {
        throw std::invalid_argument("El ratePath debe tener numSteps + 1 elementos.");
    }

    double futureValue = 0.0;
    for (size_t i = 0; i < positions.size(); ++i) {
        std::vector<double> path = positions[i].asset->generatePath(
            totalTime, numSteps, Z_matrix[i], ratePath);
        
        double finalPrice = path.back();
        
        futureValue += finalPrice * positions[i].quantity;
    }

    double discountFactor = 1.0;
    if (rateModel && !ratePath.empty()) {
        discountFactor = rateModel->discountFactorFromPath(ratePath, totalTime / numSteps);
    }

    double discountedValue = futureValue * discountFactor;
    return discountedValue - getInitialValue();
}