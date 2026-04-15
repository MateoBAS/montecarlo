#ifndef RISK_CALCULATOR_H
#define RISK_CALCULATOR_H

#include <vector>

class RiskCalculator {
private:
    std::vector<double> sortedPnLs;

public:
    RiskCalculator(const std::vector<double>& simulatedPnLs);

    double calculateVaR(double confidenceLevel) const;

    double calculateES(double confidenceLevel) const;
};

#endif // RISK_CALCULATOR_H