#pragma once
#include "Asset.h"
#include <vector>

class CouponBond : public Asset {
private:
    double faceValue;  // Valor nominal (ej. 1000 EUR)
    double couponRate; // Tasa del cupón anual (ej. 0.03 para 3%)
    double maturity;   // Años hasta el vencimiento
    double currentYTM; // TIR o tipo de interés actual del mercado
    double yieldVol;   // Volatilidad de los tipos de interés

    double calculatePrice(double yield, double timeToMaturity) const;

public:
    CouponBond(std::string name, double faceValue, double couponRate, 
               double maturity, double currentYTM, double yieldVol);
               
    std::vector<double> generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const override;
};