// --- Portfolio.cpp ---
#include "Portfolio.h"
#include "InterestRate/InterestRateModel.h"

void Portfolio::addPosition(std::shared_ptr<Asset> asset, double quantity) {
    positions.push_back({asset, quantity});
    // Acumulamos el valor inicial aquí una sola vez en lugar de hacerlo en el bucle crítico
    initialValue += asset->getInitialPrice() * quantity;
}

size_t Portfolio::getNumAssets() const {
    return positions.size();
}