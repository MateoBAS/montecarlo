#include "EuropeanOption.h"

EuropeanOption::EuropeanOption(std::string name, double optionPremium, double underlyingInitialPrice, 
                               double strike, double drift, double volatility, OptionType type)
    : Asset(std::move(name), optionPremium), underlyingInitialPrice(underlyingInitialPrice), 
    strike(strike), driftSpread(drift), volatility(volatility), type(type) {}

std::vector<double> EuropeanOption::generatePath(double totalTime, int numSteps,
                                 const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                 const std::vector<double>& ratePath) const {
    std::vector<double> path;
    path.reserve(numSteps + 1);

    // El primer valor de nuestro path de rentabilidad es el coste de la opción
    path.push_back(getInitialPrice());

    std::vector<double> underlyingPath = simulateGbmPathWithRate(
        underlyingInitialPrice, driftSpread, volatility, totalTime, numSteps, z_shocks, ratePath);

    // 1. Simulamos la trayectoria del activo subyacente
    for (size_t i = 1; i < underlyingPath.size(); ++i) {
        // Rellenamos el path con el subyacente de momento
        path.push_back(underlyingPath[i]);
    }

    // 2. Extraemos el precio final del subyacente (S_T)
    double finalUnderlyingPrice = underlyingPath.back();
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