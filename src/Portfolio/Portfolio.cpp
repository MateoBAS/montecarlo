#include "Portfolio.h"
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

double Portfolio::simulatePathPnL(double totalTime, int numSteps, const std::vector<std::vector<double>>& Z_matrix) const {
    if (Z_matrix.size() != positions.size()) {
        throw std::invalid_argument("El numero de filas en Z_matrix debe coincidir con los activos del portfolio.");
    }

    double futureValue = 0.0;
    for (size_t i = 0; i < positions.size(); ++i) {
        std::vector<double> path = positions[i].asset->generatePath(totalTime, numSteps, Z_matrix[i]);
        
        double finalPrice = path.back();
        
        futureValue += finalPrice * positions[i].quantity;
    }

    return futureValue - getInitialValue();
}