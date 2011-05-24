#ifndef MODULES_H
#define MODULES_H

/** The Modules class is the container for iLand modules (e.g. Fire, Wind, ...).
    It handles loading and invoking the functionality defined in the modules.
*/
class DisturbanceInterface; // forward
class Modules
{
public:
    Modules();
private:
    void init();
    QList<DisturbanceInterface*> mInterfaces; ///< the list stores only the active modules
};

#endif // MODULES_H
