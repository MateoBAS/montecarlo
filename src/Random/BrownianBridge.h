#ifndef BROWNIAN_BRIDGE_H
#define BROWNIAN_BRIDGE_H

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

    void transformToStandardNormals(const std::vector<double>& sobolZ, std::vector<double>& impliedZ) const;
};

#endif 