#include "CouponBond.h"
#include <cmath>

CouponBond::CouponBond(std::string name, double faceValue, double couponRate, 
                       double maturity, double currentYTM, double yieldVol)
    : Asset(name, 0.0), faceValue(faceValue), couponRate(couponRate), 
      maturity(maturity), currentYTM(currentYTM), yieldVol(yieldVol) {
    
    // Calculamos el precio sucio inicial del bono
    this->initialPrice = calculatePrice(currentYTM, maturity);
}

double CouponBond::calculatePrice(double yield, double timeToMaturity) const {
    double price = 0.0;
    int numCoupons = static_cast<int>(std::floor(timeToMaturity));
    double fractionalTime = timeToMaturity - numCoupons;
    double couponPayment = faceValue * couponRate;

    // Descontamos todos los cupones futuros
    for (int i = 1; i <= numCoupons; ++i) {
        price += couponPayment / std::pow(1.0 + yield, i + fractionalTime);
    }
    // Descontamos el pago del principal final
    price += faceValue / std::pow(1.0 + yield, numCoupons + fractionalTime);
    
    return price;
}

std::vector<double> CouponBond::generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const {
    std::vector<double> path;
    path.push_back(getInitialPrice());

    double dt = totalTime / numSteps;
    double simulatedYTM = currentYTM;

    // 1. Simulamos el movimiento de la TIR del mercado (Normal simple)
    for (int i = 0; i < numSteps; ++i) {
        simulatedYTM += yieldVol * std::sqrt(dt) * Zs[i];
    }

    // 2. Calculamos el tiempo que quedará a vencimiento tras la simulación
    double timeRemaining = maturity - totalTime;
    
    // 3. Calculamos el nuevo precio del bono con los tipos alterados
    double finalPrice = calculatePrice(simulatedYTM, timeRemaining);

    // Solo nos interesa el precio final para nuestra arquitectura actual
    path.push_back(finalPrice);

    return path;
}