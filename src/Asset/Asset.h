#pragma once
#include <string>
#include <vector>
#include <utility> // Para std::move
#include <Eigen/Dense>

class Asset {
protected:
    std::string name;
    double initialPrice;

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
    
    /*virtual std::vector<double> generatePath(double totalTime, int numSteps,
                                             const std::vector<double>& Zs,
                                             const std::vector<double>& ratePath) const = 0; // El =0 indica que tendremos que implementar esta funcion en las clases hijas obligatoriamente*/

    virtual std::vector<double> generatePath(
        double totalTime, 
        int numSteps, 
        const Eigen::Ref<const Eigen::RowVectorXd>& z_shocks,
        const std::vector<double>& ratePath) const = 0;
};