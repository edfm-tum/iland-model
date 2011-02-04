#ifndef SAPLING_H
#define SAPLING_H
#include <QtCore/QVector>
#include <QtCore/QPoint>
#include <bitset>
#include "grid.h"
#include "snag.h"

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
    void clearStatistics() { mAdded=mRecruited=mDied=mLiving=0; mSumDbhDied=0.; mAvgHeight=0.;mAvgAge=0.; mAvgDeltaHPot=mAvgHRealized=0.; }
    void clear() { mSaplingTrees.clear(); mSapBitset.reset(); }
    static void setRecruitmentVariation(const double variation) { mRecruitmentVariation = variation; }
    // access
    const QVector<SaplingTree> &saplings() const {return mSaplingTrees; }
    // actions
    void calculateGrowth(); ///< perform growth + mortality + recruitment of all saplings of this RU and species
    void addSapling(const QPoint &pos_lif);
    void clearSaplings(const QPoint &position); ///< clear  saplings on a given position (after recruitment)
    bool hasSapling(const QPoint &position) const; ///< return true if sapling is present at position
    // access to statistics
    int newSaplings() const { return mAdded; }
    int diedSaplings() const { return mDied; }
    int livingSaplings() const { return mLiving; } ///< get the number
    int recruitedSaplings() const { return mRecruited; }
    double livingStemNumber(double &rAvgDbh, double &rAvgHeight, double &rAvgAge) const; ///< returns the *represented* (Reineke's Law) number of trees (N/ha) and the mean dbh/height (cm/m)
    double averageHeight() const { return mAvgHeight; }
    double averageAge() const { return mAvgAge; }
    double averageDeltaHPot() const { return mAvgDeltaHPot; }
    double averageDeltaHRealized() const { return mAvgHRealized; }
    // carbon and nitrogen
    const CNPair &carbonLiving() const { return mCarbonLiving; } ///< state of the living
    // output maps
    void fillMaxHeightGrid(Grid<float> &grid) const;
private:
    bool growSapling(SaplingTree &tree, const double f_env_yr, Species* species);
    ResourceUnitSpecies *mRUS;
    QVector<SaplingTree> mSaplingTrees;
    std::bitset<cPxPerRU*cPxPerRU> mSapBitset;
    int mAdded; ///< number of trees added
    int mRecruited; ///< number recruited (i.e. grown out of regeneration layer)
    int mDied; ///< number of trees died
    double mSumDbhDied; ///< running sum of dbh of died trees (used to calculate detritus)
    int mLiving; ///< number of trees (cohorts!!!) currently in the regeneration layer
    double mAvgHeight; ///< average height of saplings (m)
    double mAvgAge; ///< average age of saplings (years)
    double mAvgDeltaHPot; ///< average height increment potential (m)
    double mAvgHRealized; ///< average realized height increment
    static double mRecruitmentVariation; ///< defines range of random variation for recruited trees
    CNPair mCarbonLiving;
};

#endif // SAPLING_H
