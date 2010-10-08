#ifndef SAPLING_H
#define SAPLING_H
#include <QtCore/QVector>
#include <QtCore/QPoint>
#include "grid.h"

/// SaplingTree holds information of a sapling (which represents N trees). Emphasis is on efficient storage.
class SaplingTree {
public:
    SaplingTree() { pixel=0; age.age=0; age.stress_years=0; height=0.05f; }
    bool isValid() const {return pixel!=0; }
    float *pixel; // pointer to the lifpixel the sapling lives on
    struct  { // packed two 16bit to a 32 bit integer
        short unsigned int age;  // number of consectuive years the sapling suffers from dire conditions
        short unsigned int stress_years; // (upper 16bits) + age of sapling (lower 16 bits)
    } age;
    float height; // height of the sapling in meter
private:
};

class ResourceUnitSpecies; // forward
class Species;

/// saplings from 5cm to 4m
class Sapling
{
public:
    // maintenance
    Sapling();
    void setup(ResourceUnitSpecies *masterRUS) { mRUS = masterRUS; }
    void cleanupStorage(); // maintenance operation - remove dead/recruited trees from vector
    void clearStatistics() { mAdded=mRecruited=mDied=mLiving=0; mAvgHeight=0.;mAvgAge=0.; mAvgDeltaHPot=mAvgHRealized=0.; }
    // actions
    void calculateGrowth(); ///< perform growth + mortality + recruitment of all saplings of this RU and species
    void addSapling(const QPoint &pos_lif);
    void clearSaplings(const QPoint &position); ///< clear  saplings on a given position (after recruitment)
    bool hasSapling(const QPoint &position) const; ///< return true if sapling is present at position
    // access to statistics
    int newSaplings() const { return mAdded; }
    int diedSaplings() const { return mDied; }
    int livingSaplings() const { return mLiving; }
    int recruitedSaplings() const { return mRecruited; }
    double averageHeight() const { return mAvgHeight; }
    double averageAge() const { return mAvgAge; }
    double averageDeltaHPot() const { return mAvgDeltaHPot; }
    double averageDeltaHRealized() const { return mAvgHRealized; }
    // output maps
    void fillHeightGrid(Grid<float> &grid) const;
private:
    bool growSapling(SaplingTree &tree, const double f_env_yr, Species* species);
    ResourceUnitSpecies *mRUS;
    QVector<SaplingTree> mSaplingTrees;
    int mAdded; ///< number of trees added
    int mRecruited; ///< number recruited (i.e. grown out of regeneration layer)
    int mDied; ///< number of trees died
    int mLiving; ///< number of trees currently in the regeneration layer
    double mAvgHeight; ///< average height of saplings (m)
    double mAvgAge; ///< average age of saplings (years)
    double mAvgDeltaHPot; ///< average height increment potential (m)
    double mAvgHRealized; ///< average realized height increment
};

#endif // SAPLING_H