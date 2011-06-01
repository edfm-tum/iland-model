#ifndef MODULES_H
#define MODULES_H

class DisturbanceInterface; // forward
class SetupResourceUnitInterface; // forward
class WaterInterface; // forward
class ResourceUnit; // forward
class WaterCycleData; // forward

/** The Modules class is the container for iLand modules (e.g. Fire, Wind, ...).
    It handles loading and invoking the functionality defined in the modules.
*/
class Modules
{
public:
    Modules();

    // general setup
    void setup();

    bool hasSetupResourceUnits() { return !mSetupRUs.isEmpty(); }
    // setup of resource unit specific parameters
    void setupResourceUnit(const ResourceUnit* ru);

    // functions
    void yearBegin(); ///< executes yearly initialization code for each module
    void run(); ///< execute the modules
    // water
    void calculateWater(const ResourceUnit *resource_unit, const WaterCycleData *water_data);
private:
    void init();
    QList<DisturbanceInterface*> mInterfaces; ///< the list stores only the active modules
    QList<SetupResourceUnitInterface*> mSetupRUs;
    QList<WaterInterface*> mWater;
};

#endif // MODULES_H
