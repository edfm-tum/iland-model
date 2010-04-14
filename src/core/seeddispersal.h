#ifndef SEEDDISPERSAL_H
#define SEEDDISPERSAL_H

#include "grid.h"

class SeedDispersal
{
public:
    SeedDispersal(): mSetup(false) {}
    ~SeedDispersal();
    bool isSetup() const { return mSetup; }
    void setup();
private:
    void edgeDetection();
    void distribute();
    Grid<float> mSeedMap; ///< (large) seedmap. Is filled by individual trees and then processed
    Grid<float> mSeedKernel; ///< species specific "seed kernel" (small)
    int mOffset; ///< index of center pixel of kernel
    bool mSetup;
};

#endif // SEEDDISPERSAL_H
