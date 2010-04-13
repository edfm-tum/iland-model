#ifndef MODELSETTINGS_H
#define MODELSETTINGS_H
#include <QtCore>
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
    double temperatureTau; ///< "tau"-value for delayed temperature calculation acc. to M�kela 2008
    // water
    double airDensity; // density of air [kg / m3]
    double laiThresholdForClosedStands; // for calculation of max-canopy-conductance
    double boundaryLayerConductance; // 3pg-evapotranspiration
    // site variables (for now!)
    double latitude; ///< latitude of project site in radians
    // production
    double epsilon; ///< maximum light use efficency used for the 3PG model
};

#endif // MODELSETTINGS_H
