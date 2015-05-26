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
#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

#include "grid.h"
#include "layeredgrid.h"
#include "random.h"
#include "expression.h"

#include "bbgenerations.h"

class BarkBeetleAntagonist
{
public:
    // lifecycle
    BarkBeetleAntagonist() { reset(); mArea=0.; }
    void reset() {mPopulation=1.;mBeetlePopulation=1.; }
    void clear() { mBeetlePopulation=0.; }
    /// add area (under control of the current antagonist) in ha
    void addArea(double area) { mArea+=area; }
    static void setup(); // global parameters (static)
    /// size of the antagonist-cell (m)
    static int cellsize() { return mSize; }

    /// current population of antagonists
    double population() const { return mPopulation; }
    /// the fraction of prey (bark beetles) that is eaten up by antagonists
    double feedFraction() const { return qMin(mPopulation * mRfeeding, 1.); }

    /// updates the internal beetle population counter
    void addDamage(int infested_cells) { mBeetlePopulation += infested_cells / mArea;  }

    /// antagonist activity: returns the
    double calculate();
private:
    double mPopulation; ///< current antagonist population (px per ha)
    double mBeetlePopulation; ///< size of the beetle population (px per ha)
    double mArea; ///< size of the area covered by this antagonist (ha)

    // Lotka-Volterra constants
    static double mRmortality; ///< mortality rate
    static double mRreproduction; ///< reproduction rate of the antagonist (reproduction / prey)
    static double mRfeeding; ///< feeding rate (feed / prey)
    static int mSize; ///< extent of the BBA-Cell im meters (must be multiple of 100)
};

class BarkBeetleCell
{
public:
    BarkBeetleCell() { reset(); }
    void clear() { n=0; killedYear=0; infested=false;  p_colonize=0.f; }
    void reset() {clear(); dbh=0.f; tree_stress=0.f; }
    bool isHost() const { return dbh>0.f; }
    bool isPotentialHost() const {return dbh>0.f && killedYear==0 && infested==false; }
    void setInfested(bool is_infested) { infested=is_infested; if (infested) { total_infested++; killedYear=0; n=0;} }
    void finishedSpread(int iteration) { infested=false; killedYear=iteration; killed=true; max_iteration=qMax(max_iteration, iteration); }
    bool infested;
    bool killed;
    float dbh; // the dbh of the biggest spruce on the pixel
    float tree_stress; // the stress rating of this tree
    float p_colonize; // the highest probability (0..1) that a pixel is killed
    int n; // number of cohorts that landed on the pixel
    int killedYear; // year (iteration) at which pixel was killed ??
    static void resetCounters() { total_infested=0; max_iteration=0; }
    static int total_infested;
    static int max_iteration;

};
class BarkBeetleRUCell
{
public:
    BarkBeetleRUCell(): scanned(false), generations(0.), add_sister(false),
        cold_days(0), cold_days_late(0), killed_trees(false),
        killed_pixels(0), host_pixels(0),
        smoothed_damage(0.), damage_last_year(0.), antagonist(0) {}
    /// relative damage: fraction of host pixels that died in the current or the last year
    double currentDamageFraction() { return host_pixels+killed_pixels>0? (killed_pixels)/double(host_pixels+killed_pixels): 0.; }
    double damageFraction() const { return smoothed_damage; }
    bool scanned;
    double generations;
    bool add_sister;
    int cold_days; // number of days in the winter season with t_min below a given threshold (-15 degree Celsius)
    int cold_days_late;
    bool killed_trees;
    int killed_pixels;
    int host_pixels;
    double smoothed_damage; // mean damage (3x3 resource units) of the current year
    double damage_last_year; // smoothed damage of the previous year
    BarkBeetleAntagonist *antagonist; // link to the antagonist population

};

/** Helper class manage and visualize data layers related to the barkbeetle module.
  @ingroup barkbeetle
*/
class BarkBeetleLayers: public LayeredGrid<BarkBeetleCell> {
  public:
    void setGrid(const Grid<BarkBeetleCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
};

class BarkBeetleRULayers: public LayeredGrid<BarkBeetleRUCell> {
  public:
    void setGrid(const Grid<BarkBeetleRUCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleRUCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
};



class ResourceUnit; // forward
class BarkBeetleOut; // forward
/** Main class of the bark beetle module.
  @ingroup barkbeetle
*/
class BarkBeetleModule
{
public:
    BarkBeetleModule();
    ~BarkBeetleModule();
    static double cellsize() { return 10.; }

    void setup(); ///< general setup
    void setup(const ResourceUnit *ru); ///< setup for a specific resource unit
    void loadParameters(); ///< load params from XML
    void clearGrids(); ///< reset the state of the internal grids (used for javascript based tests)
    void loadAllVegetation(); ///< scan the state of the vegetation of the full landscape

    /// main function to execute the bark beetle module
    void run(int iteration=0);

    void yearBegin();
    // properties
    void setSimulate(bool do_simulate) { mSimulate = do_simulate; }
    bool simulate() const {return mSimulate; }

    void setEnabled(bool do_set_enabled) { mEnabled = do_set_enabled; }
    bool enabled() const { return mEnabled; }
private:
    void calculateGenerations(); ///< calculate on Resource Unit level the number of potential generations
    void startSpread(); ///< beginning of a calculation
    void barkbeetleSpread(); ///< main function of bark beetle spread
    void barkbeetleKill(); ///< kill the trees on pixels marked as killed
    void scanResourceUnitTrees(const QPointF &position); ///< load tree data of the resource unit 'position' (metric) lies inside
    void calculateMeanDamage(); ///< calculate the mean damage percentage (fraction of killed pixels to host pixels)
    int mIteration;
    QString mAfterExecEvent;
    struct SBBParams {
        SBBParams(): minDbh(10.f), cohortsPerGeneration(30), cohortsPerSisterbrood(50), spreadKernelMaxDistance(100.), backgroundInfestationProbability(0.0001) {}
        float minDbh; ///< minimum dbh of spruce trees that are considered as potential hosts
        int cohortsPerGeneration; ///< 'packages' of beetles that spread from an infested pixel
        int cohortsPerSisterbrood; ///< cohorts that spread from a pixel when a full sister brood developed
        QString spreadKernelFormula; ///< formula of the PDF for the BB-spread
        double spreadKernelMaxDistance; ///< upper limit for the spread distance (the kernel is cut at this distance)
        double backgroundInfestationProbability; ///< p that a pixel gets spontaneously infested each year
        double winterMortalityBaseLevel; ///< p that a infested pixel dies out over the winter (due to antagonists, bad luck, ...)
    } params;
    struct SBBStats {
        void clear() { infestedStart=0;infestedBackground=0; maxGenerations=0;NCohortsLanded=0;NPixelsLanded=0;NCohortsSpread=0;NInfested=0;NWinterMortality=0;NTreesKilled=0;BasalAreaKilled=0.;}
        int infestedStart; // # of pixels that are infested at the beginning of an iteration
        int infestedBackground; // # of pixels that are getting active
        int maxGenerations; // maxium number of generations found this year
        int NCohortsLanded; // number of cohorts that landed on new potential host pixels
        int NPixelsLanded; // number of potential host pixels that received at least one cohort
        int NCohortsSpread; // number of pixels that are spread from infested cells
        int NInfested; // number of newly infested pixels (a subset of those who 'landed')
        int NWinterMortality; // number of (infested) pixels that died off during winter
        int NTreesKilled; // number of spruce trees killed
        double BasalAreaKilled; // sum of basal area of killed trees
    } stats;


    bool mSimulate; ///< true if bark beetle are only simulated, i.e. no trees get killed
    bool mEnabled; ///< if false, no bark beetles are simulates
    BBGenerations mGenerations;
    RandomCustomPDF mKernelPDF;
    Expression mColonizeProbability; ///< function that calculates probability of infestation for one landed beetle package given the trees' stress level
    Expression mAntagonistFormula; ///< antagonist effect: if many beetles are around, more of them are killed by antagonists; fraction of killed beetles = f(damage_fraction)
    Expression mWinterMortalityFormula; ///< temperature dependent winter mortality (more beetle die if there are more cold days)
    Grid<BarkBeetleCell> mGrid;
    Grid<BarkBeetleRUCell> mRUGrid;
    BarkBeetleLayers mLayers;
    BarkBeetleRULayers mRULayers;
    QVector<BarkBeetleAntagonist*> mAntagonists; ///< the antagonist populations

    friend class BarkBeetleScript;
    friend class BarkBeetleOut;

};



#endif // BARKBEETLEMODULE_H
