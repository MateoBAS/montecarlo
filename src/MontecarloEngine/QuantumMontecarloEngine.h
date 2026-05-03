#ifndef QUANTUM_MONTE_CARLO_ENGINE_H
#define QUANTUM_MONTE_CARLO_ENGINE_H

#include <qpp/qpp.hpp>
#include "../Asset/EuropeanOption.h"
#include "../Metrics/RiskCalculator.h"

class QuantumMontecarloEngine {
public:
    // Estimamos el valor de una opción europea
    static double estimateOptionPrice(const EuropeanOption& option, int num_qubits, double totalTime);

private:
    // Operador A: Carga la distribución de probabilidad (Log-Normal)
    static qpp::cmat setupProbabilityOperator(int n, double drift, double vol, double S0, double T);
    
    // Operador F: Implementa la función de Payoff max(S-K, 0)
    static qpp::cmat setupPayoffOperator(int n, double strike, double max_S);
};

#endif