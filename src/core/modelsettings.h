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
    bool mortalityEnabled; ///< if false, no natural (intrinsic+stress) mortality occurs
    bool regenerationEnabled; ///< if true, seed dispersal, establishment, ... is modelled
    bool carbonCycleEnabled; ///< if true, snag dynamics and soil CN cycle is modelled
    // light
    double lightExtinctionCoefficient; ///< "k" parameter (beer lambert) used for calc. of absorbed light on resourceUnit level
    double lightExtinctionCoefficientOpacity; ///< "k" for beer lambert used for opacity of single trees
    // climate
    double temperatureTau; ///< "tau"-value for delayed temperature calculation acc. to Mäkela 2008
    // water
    double airDensity; // density of air [kg / m3]
    double laiThresholdForClosedStands; // for calculation of max-canopy-conductance
    double boundaryLayerConductance; // 3pg-evapotranspiration
    // nitrogen and soil model
    bool useDynamicAvailableNitrogen; ///< if true, iLand utilizes the dynamically calculated NAvailable
    double topLayerWaterContent; ///< for the calculation of the climate-modifier of the snag/wood decay rate (Snag::calculateClimateFactors())
    // site variables (for now!)
    double latitude; ///< latitude of project site in radians
    // production
    double epsilon; ///< maximum light use efficency used for the 3PG model
    bool usePARFractionBelowGroundAllocation; ///< if true, the 'correct' version of the calculation of belowground allocation is used (default=true)


};

#endif // MODELSETTINGS_H
