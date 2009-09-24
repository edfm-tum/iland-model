#ifndef MODELSETTINGS_H
#define MODELSETTINGS_H

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
};

#endif // MODELSETTINGS_H
