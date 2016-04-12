#ifndef SAPLINGS_H
#define SAPLINGS_H

#include "grid.h"
#include "snag.h"


struct SaplingTree {
    SaplingTree() { clear(); }
    short unsigned int age;  // number of consectuive years the sapling suffers from dire conditions
    short signed int species_index; // index of the species within the resource-unit-species container
    unsigned char stress_years; // (upper 16bits) + age of sapling (lower 16 bits)
    unsigned char flags;
    float height; // height of the sapling in meter
    bool is_occupied() const { return height>0.f; }
    void clear()  {  age=0; species_index=-1; stress_years=0; flags=0; height=0.f;  }
    void setSapling(const float h_m, const int age_yrs, const int species_idx) { height=h_m; age=static_cast<short unsigned int>(age_yrs); stress_years=0; species_index=static_cast<short signed int>(species_idx); }
};
#define NSAPCELLS 5
struct SaplingCell {
    enum ECellState { CellInvalid=0, CellFree=1, CellFull=2};
    SaplingCell() {
        state=CellInvalid;
    }
    ECellState state;
    SaplingTree saplings[NSAPCELLS];
    void checkState() { if (state==CellInvalid) return;
                        bool free = false;
                        for (int i=0;i<NSAPCELLS;++i) {
                            // locked for all species, if a sapling of one species >1.3m
                            if (saplings[i].height>1.3f) {state = CellFull; return; }
                            // locked, if all slots are occupied.
                            if (!saplings[i].is_occupied())
                                free=true;
                        }
                        state = free? CellFree : CellFull;
                      }
    /// get an index to an open slot in the cell, or -1 if all slots are occupied
    int free_index() {
        for (int i=0;i<NSAPCELLS;++i)
            if (!saplings[i].is_occupied())
                return i;
        return -1;
    }
    /// add a sapling to this cell, return a pointer to the tree on success, or 0 otherwise
    SaplingTree *addSapling(const float h_m, const int age_yrs, const int species_idx) {
        int idx = free_index();
        if (idx==-1)
            return 0;
        saplings[idx].setSapling(h_m, age_yrs, species_idx);
        return &saplings[idx];
    }
    /// return the maximum height on the pixel
    float max_height() { if (state==CellInvalid) return 0.f;
                         float h_max = 0.f;
                         for (int i=0;i<NSAPCELLS;++i)
                             h_max = std::max(saplings[i].height, h_max);
                         return h_max;
                       }
    /// return the sapling tree of the requested species, or 0
    SaplingTree *sapling(int species_index) {
        if (state==CellInvalid) return 0;
        for (int i=0;i<NSAPCELLS;++i)
            if (saplings[i].species_index == species_index)
                return &saplings[i];
        return 0;
    }
};
class ResourceUnit;
class Saplings;

/** The SaplingStat class stores statistics on the resource unit x species level.
 */
class SaplingStat
{
public:
    SaplingStat() { clearStatistics(); }
    void clearStatistics();
    void calculate(const Species *species, ResourceUnit *ru);
    // actions
    void addCarbonOfDeadSapling(float dbh) { mDied++; mSumDbhDied+=dbh; }

    // access to statistics
    int newSaplings() const { return mAdded; }
    int diedSaplings() const { return mDied; }
    int livingSaplings() const { return mLiving; } ///< get the number
    int recruitedSaplings() const { return mRecruited; }
    ///  returns the *represented* (Reineke's Law) number of trees (N/ha) and the mean dbh/height (cm/m)
    double livingStemNumber(double &rAvgDbh, double &rAvgHeight, double &rAvgAge) const;

    double averageHeight() const { return mAvgHeight; }
    double averageAge() const { return mAvgAge; }
    double averageDeltaHPot() const { return mAvgDeltaHPot; }
    double averageDeltaHRealized() const { return mAvgHRealized; }
    /// return the number of trees represented by one sapling of the current species and given 'height'
    double representedStemNumber(float height) const;
    // carbon and nitrogen
    const CNPair &carbonLiving() const { return mCarbonLiving; } ///< state of the living
    const CNPair &carbonGain() const { return mCarbonGain; } ///< state of the living

private:
    int mAdded; ///< number of trees added
    int mRecruited; ///< number recruited (i.e. grown out of regeneration layer)
    int mDied; ///< number of trees died
    double mSumDbhDied; ///< running sum of dbh of died trees (used to calculate detritus)
    int mLiving; ///< number of trees (cohorts!!!) currently in the regeneration layer
    double mAvgHeight; ///< average height of saplings (m)
    double mAvgAge; ///< average age of saplings (years)
    double mAvgDeltaHPot; ///< average height increment potential (m)
    double mAvgHRealized; ///< average realized height increment
    CNPair mCarbonLiving;
    CNPair mCarbonGain; ///< net growth (kg / ru) of saplings

    friend class Saplings;

};
/** The Saplings class the container for the establishment and sapling growth in iLand.
 *
*/
class Saplings
{
public:
    Saplings();
    void setup();
    // main functions
    void establishment(const ResourceUnit *ru);
    void saplingGrowth(const ResourceUnit *ru);

    // access
    const Grid<SaplingCell> &grid() const { return mGrid; }
    SaplingCell *cell(QPoint lif_coords)  { SaplingCell *s=mGrid.ptr(lif_coords.x(), lif_coords.y()); if (s && s->state!=SaplingCell::CellInvalid) return s; else return 0; }

    static void setRecruitmentVariation(const double variation) { mRecruitmentVariation = variation; }
    static void updateBrowsingPressure();

private:
    bool growSapling(const ResourceUnit *ru, SaplingTree &tree, int isc, float dom_height, float lif_value);
    Grid<SaplingCell> mGrid;
    static double mRecruitmentVariation;
    static double mBrowsingPressure;
};

#endif // SAPLINGS_H
