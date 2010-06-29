#ifndef SEEDDISPERSAL_H
#define SEEDDISPERSAL_H

#include "grid.h"
class Species;

class SeedDispersal
{
public:
    SeedDispersal(Species *species=0): mIndexFactor(10), mSetup(false), mSpecies(species) {}
    ~SeedDispersal();
    bool isSetup() const { return mSetup; }
    void setup();
    // access
    const Grid<float> seedMap() const { return mSeedMap; } ///< access to the seedMap
    const Species *species() const {return mSpecies; }
    /// setMatureTree is called by individual (mature) trees. This actually fills the initial state of the seed map.
    void setMatureTree(const QPoint &lip_index) { mSeedMap.valueAtIndex((lip_index.x()-mIndexOffset.x())/mIndexFactor, (lip_index.y()-mIndexOffset.y())/mIndexFactor)=1.f; }
    // operations
    void clear(); ///< clears the grid
    void execute(); ///< execute the seed dispersal
    void edgeDetection(); ///< phase 1: detect edges in the image
    void distribute(); ///< phase 2: distribute seeds
    // debug and helpers
    void loadFromImage(const QString &fileName); ///< debug function...
private:
    void createKernel(Grid<float> &kernel, const float max_seed); ///< initializes / creates the kernel
    double treemig(const double &distance);
    double treemig_distanceTo(const double value);
    double mTM_as1, mTM_as2, mTM_ks; ///< seed dispersal paramaters (treemig)
    double mTM_fecundity_cell; ///< maximum seeds per source cell
    double mTM_occupancy; ///< seeds required per destination regeneration pixel
    double mNonSeedYearFraction; ///< fraction of the seed production in non-seed-years
    int mIndexFactor; ///< multiplier between light-pixel-size and seed-pixel-size
    QPoint mIndexOffset; ///< offset of (0/0) in light-pixel coordinates
    Grid<float> mSeedMap; ///< (large) seedmap. Is filled by individual trees and then processed
    Grid<float> mKernelSeedYear; ///< species specific "seed kernel" (small) for seed years
    Grid<float> mKernelNonSeedYear; ///< species specific "seed kernel" (small) for non-seed-years
    bool mSetup;
    Species *mSpecies;
};

#endif // SEEDDISPERSAL_H
