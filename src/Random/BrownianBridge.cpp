#include "BrownianBridge.h"
#include <cmath>
#include <utility>

BrownianBridge::BrownianBridge(int steps, double T) : numSteps(steps), totalT(T) {
    leftIdx.resize(steps);
    rightIdx.resize(steps);
    bridgeIdx.resize(steps);
    leftWeight.resize(steps);
    rightWeight.resize(steps);
    stdDev.resize(steps);

    std::vector<double> t(steps + 1);
    for(int i = 0; i <= steps; ++i) {
        t[i] = i * (T / steps);
    }

    std::vector<bool> computed(steps + 1, false);
    computed[0] = true;
    computed[steps] = true;

    bridgeIdx[0] = steps;
    leftIdx[0] = 0;
    rightIdx[0] = 0;
    stdDev[0] = std::sqrt(t[steps]);
    leftWeight[0] = 0.0;
    rightWeight[0] = 0.0;

    int sobolIndex = 1;
    std::vector<std::pair<int, int>> intervals = {{0, steps}};

    while(sobolIndex < steps) {
        std::vector<std::pair<int, int>> newIntervals;
        for(const auto& p : intervals) {
            int l = p.first;
            int r = p.second;
            int mid = l + (r - l) / 2;

            if (mid > l && mid < r && !computed[mid]) {
                bridgeIdx[sobolIndex] = mid;
                leftIdx[sobolIndex] = l;
                rightIdx[sobolIndex] = r;

                double tl = t[l];
                double tr = t[r];
                double tm = t[mid];

                leftWeight[sobolIndex] = (tr - tm) / (tr - tl);
                rightWeight[sobolIndex] = (tm - tl) / (tr - tl);
                stdDev[sobolIndex] = std::sqrt(((tm - tl) * (tr - tm)) / (tr - tl));

                computed[mid] = true;
                sobolIndex++;

                newIntervals.push_back({l, mid});
                newIntervals.push_back({mid, r});
            }
        }
        intervals = newIntervals;
    }
}

void BrownianBridge::transformToStandardNormals(const std::vector<double>& sobolZ, std::vector<double>& impliedZ) const {
    std::vector<double> W(numSteps + 1, 0.0);

    W[bridgeIdx[0]] = stdDev[0] * sobolZ[0];

    for(int i = 1; i < numSteps; ++i) {
        int j = bridgeIdx[i];
        int l = leftIdx[i];
        int r = rightIdx[i];
        W[j] = leftWeight[i] * W[l] + rightWeight[i] * W[r] + stdDev[i] * sobolZ[i];
    }

    double dtSqrt = std::sqrt(totalT / numSteps);
    for(int i = 0; i < numSteps; ++i) {
        impliedZ[i] = (W[i+1] - W[i]) / dtSqrt;
    }
}