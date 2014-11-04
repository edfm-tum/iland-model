#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

#include "grid.h"

class BarkBeetleCell
{
    int n;
};

class ResourceUnit; // forward

class BarkBeetleModule
{
public:
    BarkBeetleModule();
    static double cellsize() { return 10.; }

    void setup(); ///< general setup
    void setup(const ResourceUnit *ru); ///< setup for a specific resource unit

    /// main function to execute the bark beetle module
    void run();

    void yearBegin();
private:
    Grid<BarkBeetleCell> mGrid;

};

#endif // BARKBEETLEMODULE_H
