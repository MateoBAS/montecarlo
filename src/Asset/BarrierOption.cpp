#include "BarrierOption.h"

BarrierOption::BarrierOption(std::string name, double optionPremium, double underlyingInitialPrice, 
                             double strike, double barrier, double drift, double volatility)
    : Asset(std::move(name), optionPremium), underlyingInitialPrice(underlyingInitialPrice), 
      strike(strike), barrier(barrier), drift(drift), volatility(volatility) {}

std::vector<double> BarrierOption::generatePath(double totalTime, int numSteps, const std::vector<double>& Zs) const {
    std::vector<double> path;
    path.reserve(numSteps + 1);

    // El primer valor del path es el coste (prima)
    path.push_back(getInitialPrice());

    std::vector<double> underlyingPath =
        simulateGbmPath(underlyingInitialPrice, drift, volatility, totalTime, numSteps, Zs);

    bool knockedOut = false;

    // 1. Simulamos la trayectoria paso a paso
    for (size_t i = 1; i < underlyingPath.size(); ++i) {
        double currentUnderlyingPrice = underlyingPath[i];

        // Comprobamos si el subyacente ha tocado o superado la barrera (Up-and-Out)
        if (currentUnderlyingPrice >= barrier) {
            knockedOut = true;
        }

        path.push_back(currentUnderlyingPrice);
    }

    // 2. Calculamos el Payoff
    double payoff = 0.0;

    // Si NO ha tocado la barrera, paga como una Call estándar
    if (!knockedOut) {
        double finalUnderlyingPrice = underlyingPath.back();
        payoff = std::max(finalUnderlyingPrice - strike, 0.0);
    }
    // Si knockedOut es true, el payoff se queda en 0.0 automáticamente

    // 3. Reemplazamos el último valor con el pago real a vencimiento
    path.back() = payoff;

    return path;
}