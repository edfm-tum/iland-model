/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef SAPLINGS_H
#define SAPLINGS_H

#include "grid.h"
#include "snag.h"
#include "model.h"
#include <QRectF>
class ResourceUnitSpecies; // forward
class ResourceUnit; // forward

struct SaplingTree {
    SaplingTree() { clear(); }
    short unsigned int age;  // number of consectuive years the sapling suffers from dire conditions
    short signed int species_index; // index of the species within the resource-unit-species container
    unsigned char stress_years; // number of consecutive years that a sapling suffers from stress
    unsigned char flags; // flags, e.g. whether sapling stems from sprouting
    float height; // height of the sapling in meter
    bool is_occupied() const { return height>0.f; }
    void clear()  {  age=0; species_index=-1; stress_years=0; flags=0; height=0.f;  }
    void setSapling(const float h_m, const int age_yrs, const int species_idx) { height=h_m;
                                                                                 age=static_cast<short unsigned int>(age_yrs);
                                                                                 stress_years=0;
                                                                                 species_index=static_cast<short signed int>(species_idx); }
    // flags
    bool is_sprout() const { return flags & 1; }
    void set_sprout(const bool sprout) {if (sprout) flags |= 1; else flags &= (1 ^ 0xffffff ); }
    // get resource unit species of the sapling tree
    ResourceUnitSpecies *resourceUnitSpecies(const ResourceUnit *ru);
};
#define NSAPCELLS 5
struct SaplingCell {
    enum ECellState { CellInvalid=0, ///< not stockable (outside project area)
                      CellEmpty=1,   ///< the cell has no slots occupied (no saplings on the cell)
                      CellGrass=2,   ///< the cell is empty and has grass cover (see grass module)
                      CellFree=3,    ///< seedlings may establish on the cell (at least one slot occupied)
                      CellFull=4};   ///< cell is full (no establishment) (either all slots used or one slot > 1.3m)
    SaplingCell() {
        state=CellInvalid;
    }
    ECellState state;
    SaplingTree saplings[NSAPCELLS];
    /// returns true if establishment is allowed for the cell
    bool hasFreeSlots() const {return state>CellInvalid && state<CellFull; }
    void checkState() { if (state==CellInvalid) return;
                        bool free = false;
                        bool occupied=false;
                        for (int i=0;i<NSAPCELLS;++i) {
                            // locked for all species, if a sapling of one species >1.3m
                            if (saplings[i].height>1.3f) {state = CellFull; return; }
                            occupied |= saplings[i].is_occupied();
                            // locked, if all slots are occupied.
                            if (!saplings[i].is_occupied())
                                free=true;
                        }
                        state = free? (occupied? CellEmpty: CellFree) : CellFull;
                      }
    /// get an index to an open slot in the cell, or -1 if all slots are occupied
    int free_index() {
        for (int i=0;i<NSAPCELLS;++i)
            if (!saplings[i].is_occupied())
                return i;
        return -1;
    }
    /// count the number of occupied slots on the pixel
    int n_occupied() {
        int n=0;
        for (int i=0;i<NSAPCELLS;++i)
            n+=saplings[i].is_occupied();
        return n;
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
    bool has_new_saplings() { if (state==CellInvalid) return 0.f;
                        for (int i=0;i<NSAPCELLS;++i)
                            if (saplings[i].is_occupied() && saplings[i].age<2)
                                return true;
                        return false;
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
    /// calculate statistics (and carbon flows) for the saplings of species 'species' on 'ru'.
    void calculate(const Species *species, ResourceUnit *ru);
    // actions
    void addCarbonOfDeadSapling(float dbh) { mDied++; mSumDbhDied+=static_cast<double>(dbh);  }

    // access to statistics
    int newSaplings() const { return mAdded; }
    int diedSaplings() const { return mDied; }
    int livingCohorts() const { return mLiving; } ///< get the number of cohorts
    double livingSaplings() const { return mLivingSaplings; } ///< number of individual trees in the regen layer (using Reinekes R), with h>1.3m
    double livingSaplingsSmall() const { return mLivingSmallSaplings; }
    int recruitedSaplings() const { return mRecruited; }
    ///  returns the *represented* (Reineke's Law) number of trees (N/ha) and the mean dbh/height (cm/m)
    double livingStemNumber(const Species *species, double &rAvgDbh, double &rAvgHeight, double &rAvgAge) const;

    double averageHeight() const { return mAvgHeight; }
    double averageAge() const { return mAvgAge; }
    double averageDeltaHPot() const { return mAvgDeltaHPot; }
    double averageDeltaHRealized() const { return mAvgHRealized; }
    double leafArea() const { return mLeafArea; }
    void setLeafArea(double leaf_area){ mLeafArea = leaf_area; }
    double leafAreaIndex() const {return mLeafAreaIndex; }
    double basalArea() const { return mBasalArea; }
    // carbon and nitrogen
    const CNPair &carbonLiving() const { return mCarbonLiving; } ///< state of the living
    const CNPair &carbonGain() const { return mCarbonGain; } ///< state of the living

private:
    int mAdded; ///< number of tree cohorts added
    int mRecruited; ///< number of cohorts recruited (i.e. grown out of regeneration layer)
    int mDied; ///< number of tree cohorts died
    double mSumDbhDied; ///< running sum of dbh of died trees (used to calculate detritus)
    int mLiving; ///< number of trees (cohorts!!!) currently in the regeneration layer
    int mCohortsWithDbh; ///< number of cohorts that are >1.3m
    double mLivingSaplings; ///< number of individual trees in the regen layer (using Reinekes R), with h>1.3m
    double mLivingSmallSaplings; ///< number of individual trees of cohorts < 1.3m height
    double mAvgHeight; ///< average height of saplings (m)
    double mAvgAge; ///< average age of saplings (years)
    double mAvgDeltaHPot; ///< average height increment potential (m)
    double mAvgHRealized; ///< average realized height increment
    double mLeafArea; ///< total leaf area (on all pixels of the resource unit)
    double mLeafAreaIndex; ///< leaf area index (m2/m2)
    double mBasalArea; ///< basal area (m2) of saplings
    CNPair mCarbonLiving; ///< kg Carbon (kg/ru) of saplings
    CNPair mCarbonGain; ///< net growth (kg / ru) of saplings
    double mCarbonOfRecruitedTrees; ///< carbon that is added when trees >4m are created

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
    void calculateInitialStatistics(const ResourceUnit *ru);
    // main functions
    void establishment(const ResourceUnit *ru);
    void saplingGrowth(const ResourceUnit *ru);

    /// run the simplified grass cover for a RU
    void simplifiedGrassCover(const ResourceUnit *ru);

    /// calculate the top height of the sapling layer
    double topHeight(const ResourceUnit *ru) const;

    // access
    /// return the SaplingCell (i.e. container for the ind. saplings) for the given 2x2m coordinates
    /// if 'only_valid' is true, then 0 is returned if no living saplings are on the cell
    /// 'rRUPtr' is a pointer to a RU-ptr: if provided, a pointer to the resource unit is stored
    SaplingCell *cell(QPoint lif_coords, bool only_valid=true, ResourceUnit **rRUPtr=0);
    /// clear/kill all saplings within the rectangle given by 'rectangle'.
    /// If 'remove_biomass' is true, then the biomass is extracted (e.g. burnt), otherwise they are moved to soil
    void clearSaplings(const QRectF &rectangle, const bool remove_biomass, bool resprout);
    /// clear all saplings on a given cell 's' (if 'remove_biomass' is true: biomass removed from system (e.g. burnt))
    void clearSaplings(SaplingCell *s, ResourceUnit *ru, const bool remove_biomass, bool resprout);
    /// clear all saplings, biomass is removed (not routed to the soil layer)
    void clearAllSaplings();

    /// generate vegetative offspring from 't' (sprouts)
    int addSprout(const Tree *t, bool tree_is_removed);

    static void setRecruitmentVariation(const double variation) { mRecruitmentVariation = variation; }
    static void updateBrowsingPressure();

private:
    bool growSapling(const ResourceUnit *ru, SaplingCell &scell, SaplingTree &tree, int isc, HeightGridValue &hgv, float lif_value, int cohorts_on_px);
    //Grid<SaplingCell> mGrid;
    static double mRecruitmentVariation;
    static double mBrowsingPressure;
};


/** SaplingCellRunner is a helper class to access all SaplingCell that
 * are located on a given "stand" (in the stand grid)
  */
class MapGrid; // forward
class SaplingCellRunner
{
public:
    SaplingCellRunner(const int stand_id, const MapGrid *stand_grid=0);
    ~SaplingCellRunner();
    SaplingCell *next();
    ResourceUnit *ru() const { return mRU; }
    QPointF currentCoord() const;
private:
    GridRunner<float> *mRunner;
    ResourceUnit *mRU;
    const MapGrid *mStandGrid;
    int mStandId;
};

#endif // SAPLINGS_H
