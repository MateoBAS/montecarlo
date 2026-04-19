#ifndef BROWNIAN_BRIDGE_H
#define BROWNIAN_BRIDGE_H

//#pragma once
#include <vector>

class BrownianBridge {
private:
    int numSteps;
    double totalT;
    
    std::vector<int> leftIdx;
    std::vector<int> rightIdx;
    std::vector<int> bridgeIdx;
    std::vector<double> leftWeight;
    std::vector<double> rightWeight;
    std::vector<double> stdDev;

public:
    BrownianBridge(int steps, double T);

    // Método principal de transformación
    void transformToStandardNormals(const std::vector<double>& sobolZ, std::vector<double>& impliedZ) const;
};

#endif // BROWNIAN_BRIDGE_H