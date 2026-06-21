#pragma once
#include <string>
#include <vector>
#include <utility>
#include <Eigen/Dense>

class Asset {
protected:
    std::string name;
    double initialPrice;

    double simulateGbmEvolution(double startPrice, double driftSpread, double volatility,
                                double totalTime, int numSteps,
                                const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                const std::vector<double>& ratePath,
                                std::vector<double>* pathOut) const;

    double simulateGbmTerminal(double startPrice, double driftSpread, double volatility,
                               double totalTime, int numSteps,
                               const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                               const std::vector<double>& ratePath) const;

    std::vector<double> simulateGbmPath(double startPrice, double drift, double volatility,
                                          double totalTime, int numSteps,
                                          const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks) const;

    std::vector<double> simulateGbmPathWithRate(double startPrice, double driftSpread, double volatility,
                                                double totalTime, int numSteps,
                                                const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                                const std::vector<double>& ratePath) const;

public:
    Asset(std::string name, double initialPrice)
        : name(std::move(name)), initialPrice(initialPrice) {}

    virtual ~Asset() = default;

    std::string getName() const { return name; }
    double getInitialPrice() const { return initialPrice; }

    // Hot path: valor terminal descontable (payoff o precio final del activo).
    virtual double simulateFinalValue(double totalTime, int numSteps,
                                      const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                      const std::vector<double>& ratePath) const = 0;

    // Cold path: trayectoria completa o mínima para tests y análisis.
    virtual std::vector<double> generatePath(double totalTime, int numSteps,
                                             const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
                                             const std::vector<double>& ratePath) const = 0;

    // Indica si generatePath materializa precios intermedios (p. ej. GBM).
    virtual bool recordsIntermediatePrices() const { return false; }
};
