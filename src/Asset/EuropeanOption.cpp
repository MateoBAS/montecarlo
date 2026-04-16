#include "EuropeanOption.h"
#include <cmath>

EuropeanOption::EuropeanOption(std::string name, double optionPremium, double underlyingInitialPrice, 
                               double strike, double drift, double volatility, OptionType type)
    : Asset(std::move(name), optionPremium), underlyingInitialPrice(underlyingInitialPrice), 
      strike(strike), drift(drift), volatility(volatility), type(type) {}

std::vector<double> EuropeanOption::generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const {
    std::vector<double> path;
    path.reserve(numSteps + 1); 
    
    // El primer valor de nuestro path de rentabilidad es el coste de la opción
    path.push_back(getInitialPrice()); 
    
    double dt = totalTime / numSteps;
    double variance_penalty = 0.5 * volatility * volatility;
    double drift_term = (drift - variance_penalty) * dt;
    double vol_term = volatility * std::sqrt(dt);
    
    double currentUnderlyingPrice = underlyingInitialPrice;
    
    // 1. Simulamos la trayectoria del activo subyacente
    for (int i = 0; i < numSteps; ++i) {
        double exponent = drift_term + vol_term * Zs[i];
        currentUnderlyingPrice *= std::exp(exponent);
        
        // Rellenamos el path con el subyacente de momento
        path.push_back(currentUnderlyingPrice); 
    }
    
    // 2. Extraemos el precio final del subyacente (S_T)
    double finalUnderlyingPrice = path.back();
    double payoff = 0.0;
    
    // 3. Aplicamos la fórmula del derivado
    if (type == OptionType::Call) {
        payoff = std::max(finalUnderlyingPrice - strike, 0.0);
    } else {
        payoff = std::max(strike - finalUnderlyingPrice, 0.0);
    }
    
    // 4. Reemplazamos el último valor del path con el pago real a vencimiento
    // Así, cuando Portfolio mire path.back(), verá el valor de la opción, no de la acción.
    path.back() = payoff;
    
    return path;
}