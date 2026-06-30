
#include "Portfolio.h"
#include "InterestRate/InterestRateModel.h"

void Portfolio::addPosition(std::shared_ptr<Asset> asset, double quantity) {
    positions.push_back({asset, quantity});

    initialValue += asset->getInitialPrice() * quantity;
}

size_t Portfolio::getNumAssets() const {
    return positions.size();
}