/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

#ifndef SEEDDISPERSAL_H
#define SEEDDISPERSAL_H
#include <QHash>
#include "grid.h"
class Species;
class Tree;

class SeedDispersal
{
public:
    SeedDispersal(Species *species=0): mIndexFactor(10), mSaplingMapCreated(false), mSetup(false), mSpecies(species)  {}
    ~SeedDispersal();
    /// internal data structure for tiled seed dispersal
    struct SeedDispTask {
        SeedDispersal *dispersalObj;
        QPoint tileIndex;
        int tileWidth;
        int tileHeight;
    };

    /// the cell size of seed dispersal is 20m
    static constexpr int cellSize() { return 20; }
    bool isSetup() const { return mSetup; }
    void setup();
    //
    static void setupExternalSeeds();
    static void finalizeExternalSeeds();
    /// setup tiles for parallel execution
    static void prepareParallelization(const Grid<float> &example_seed_map);
    // access
    const Grid<float> &seedMap() const { return mSeedMap; } ///< access to the seedMap
    const Species *species() const {return mSpecies; }

    /// setMatureTree is called by individual (mature) trees. This actually fills the initial state of the seed map.
    void setMatureTree(const QPoint &lip_index, double leaf_area) {
       mSourceMap.valueAtIndex(lip_index.x()/mIndexFactor, lip_index.y()/mIndexFactor) += leaf_area;
    }

    /// flags pixel at 'lip_index' that seeds should be produced. Called by saplings in the regeneration layer.
    void setSaplingTree(const QPoint &lip_index, float leaf_area);

    /// extra seed rain of serotinous species at 'position_index'
    void seedProductionSerotiny(const Tree *tree);

    // operations
    void newYear(); ///< initial values at the beginning of the year for the grid
    void clearSaplingMap(); ///< clear

    /// Main function - run the seed dispersal
    void execute();

    /// run seed dispersal (tiled mode)
    void executeTiled(const SeedDispTask &task);

    // debug and helpers
    /// return true if tiles should be used
    static bool isTiled() { return mSeedDispTasks.size() > 0; }

    void loadFromImage(const QString &fileName); ///< debug function...
    void dumpMapNextYear(QString file_name) { mDumpNextYearFileName = file_name; }
    void runTest(int which_one, int times);
private:
    void createKernel(Grid<float> &kernel, const float scale_area); ///< initializes / creates the kernel
    double setupLDD(); ///< initialize long distance seed dispersal
    double treemig(const double &distance);
    // numerical integration of the treemig function up to a radius 'max_distance'
    double treemig_centercell(const double &max_distance);
    double treemig_distanceTo(const double value);

    /// check serotiny and prepare
    void checkSerotiny();

    /// do the actual seed distribution processing
    void distributeSeeds(Grid<float> *seed_map=0);
    void distributeSeedsFast();
    /// worker function for tiled seed distribution
    void distributeSeedsTiled(const SeedDispersal::SeedDispTask &task);

    /// function for long-distance-dispersal
    void distributeFinalize();

    /// external seeds on full area (in case of low probability)
    void addExternalBackgroundSeeds(Grid<float> &map, double background_value);


    double mTM_as1, mTM_as2, mTM_ks; ///< seed dispersal paramaters (treemig)
    double mTM_fecundity_cell; ///< maximum seeds per source cell
    double mTM_occupancy; ///< seeds required per destination regeneration pixel
    double mNonSeedYearFraction; ///< fraction of the seed production in non-seed-years
    double mKernelThresholdArea, mKernelThresholdLDD; ///< value of the kernel function that is the threhold for full coverage and LDD, respectively
    int mIndexFactor; ///< multiplier between light-pixel-size and seed-pixel-size
    Grid<float> mSeedMap; ///< (large) seedmap. Is filled by individual trees and then processed
    Grid<float> mSourceMap; ///< (large) seedmap used to denote the sources
    Grid<float> mKernelSeedYear; ///< species specific "seed kernel" (small) for seed years
    Grid<float> mKernelNonSeedYear; ///< species specific "seed kernel" (small) for non-seed-years
    Grid<float> mKernelSerotiny; ///< seed kernel for extra seed rain
    Grid<float> mSeedMapSerotiny; ///< seed map that keeps track of serotiny events (only for serotinous species)
    Grid<float> mSaplingSourceMap; ///< seed map that collects seed distribution from sapling trees
    bool mSaplingMapCreated; ///< flag that indicates if a map for saplings has been created
    QVector<double> mLDDDistance; ///< long distance dispersal distances (e.g. the "rings")
    QVector<double> mLDDDensity;  ///< long distance dispersal # of cells that should be affected in each "ring"
    int mLDDRings; ///< # of rings (with equal probability) for LDD
    float mLDDSeedlings; ///< each LDD pixel has this probability
    bool mHasPendingSerotiny; ///< true if active (unprocessed) pixels are on the extra-serotiny map
    bool mSetup;
    Species *mSpecies;
    bool mDumpSeedMaps; ///< if true, seedmaps are stored as images
    bool mHasExternalSeedInput; ///< if true, external seeds are modelled for the species
    QString mDumpNextYearFileName; ///< debug output - dump of the content of the grid to a file during the next execution
    unsigned int mExternalSeedDirection; ///< direction of external seeds
    int mExternalSeedBuffer; ///< how many 20m pixels away from the simulation area should the seeding start?
    double mExternalSeedBackgroundInput; ///< background propability for this species; if set, then a certain seed availability is provided for the full area
    // external seeds
    Grid<float> mExternalSeedMap; ///< for more complex external seed input, this map holds that information
    void setupExternalSeedsForSpecies(Species *species); ///< setup of special external seed input
    static Grid<float> *mExternalSeedBaseMap; ///< static intermediate data while setting up external seeds
    static QHash<QString, QVector<double> > mExtSeedData; ///< holds definition of species and percentages for external seed input
    static int mExtSeedSizeX, mExtSeedSizeY; ///< size of the sectors used to specify external seed input
    // tiling
    mutable std::atomic<int> mTilesProcessed; ///< counter to check when a species is finished
    int mTotalTiles {0}; // number of tiles (for this species)
    static constexpr int tileSize = 64;

    static QVector<SeedDispTask> mSeedDispTasks;


    friend class SpeciesSet;
};

#endif // SEEDDISPERSAL_H
