#ifndef MODELSETTINGS_H
#define MODELSETTINGS_H

#include "expression.h"
class ModelSettings
{
public:
    ModelSettings();
    void loadModelSettings();
    void print();
    // list of settings
    // general on/off switches
    bool growthEnabled; ///< if false, trees will apply/read light patterns, but do not grow
    bool mortalityEnabled;
    // light
    double lightExtinctionCoefficient; ///< "k" parameter (beer lambert) used for calc. of absorbed light on resourceUnit level
    double lightExtinctionCoefficientOpacity; ///< "k" for beer lambert used for opacity of single trees
    // climate
    double temperatureTau; ///< "tau"-value for delayed temperature calculation acc. to Mäkela 2008
    // light model
    QSharedPointer<Expression> lightResponse;
    // water
    double heatCapacityAir;
    double airDensity;
    // site variables (for now!)
    double latitude; ///< latitude of project site in radians
    double nitrogenAvailable; ///< nitrogen content (kg/m2/year) -> will be moved!
    double waterholdingCapacity; ///< waterholding capacity (mm) -> will be removed!!!
    // production
    double epsilon; ///< maximum light use efficency used for the 3PG model
};

#endif // MODELSETTINGS_H
