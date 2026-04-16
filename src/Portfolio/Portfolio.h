#pragma once
#include <vector>
#include <memory>
#include "../Asset/Asset.h"

struct Position {
    std::shared_ptr<Asset> asset;
    double quantity;
};

class Portfolio {
private:
    std::vector<Position> positions;

public:
    void addPosition(std::shared_ptr<Asset> asset, double quantity);
    
    double getInitialValue() const; 
    size_t getNumAssets() const;

    double simulatePathPnL(double totalTime, int numSteps, const std::vector<std::vector<double>>& Z_matrix) const;
};