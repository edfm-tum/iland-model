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

#ifndef SEEDDISPERSAL_H
#define SEEDDISPERSAL_H
#include <QHash>
#include "grid.h"
class Species;

class SeedDispersal
{
public:
    SeedDispersal(Species *species=0): mIndexFactor(10), mSetup(false), mSpecies(species) {}
    ~SeedDispersal();
    bool isSetup() const { return mSetup; }
    void setup();
    //
    static void setupExternalSeeds();
    static void finalizeExternalSeeds();
    // access
    const Grid<float> &seedMap() const { return mSeedMap; } ///< access to the seedMap
    const Species *species() const {return mSpecies; }
    /// setMatureTree is called by individual (mature) trees. This actually fills the initial state of the seed map.
    void setMatureTree(const QPoint &lip_index) { mSeedMap.valueAtIndex(lip_index.x()/mIndexFactor, lip_index.y()/mIndexFactor)=1.f; }
    /// extra seed rain of serotinous species at 'position_index'
    void seedProductionSerotiny(const QPoint &position_index);

    // operations
    void clear(); ///< clears the grid
    void execute(); ///< execute the seed dispersal
    bool edgeDetection(Grid<float> *seed_map = 0); ///< phase 1: detect edges in the image; returns false if *no* pixel is 'lit'
    void distribute(Grid<float> *seed_map = 0); ///< phase 2: distribute seeds
    // debug and helpers
    void loadFromImage(const QString &fileName); ///< debug function...
    void dumpMapNextYear(QString file_name) { mDumpNextYearFileName = file_name; }
private:
    void createKernel(Grid<float> &kernel, const double max_seed); ///< initializes / creates the kernel
    double treemig(const double &distance);
    double treemig_distanceTo(const double value);
    double mTM_as1, mTM_as2, mTM_ks; ///< seed dispersal paramaters (treemig)
    double mTM_fecundity_cell; ///< maximum seeds per source cell
    double mTM_occupancy; ///< seeds required per destination regeneration pixel
    double mNonSeedYearFraction; ///< fraction of the seed production in non-seed-years
    int mIndexFactor; ///< multiplier between light-pixel-size and seed-pixel-size
    Grid<float> mSeedMap; ///< (large) seedmap. Is filled by individual trees and then processed
    Grid<float> mKernelSeedYear; ///< species specific "seed kernel" (small) for seed years
    Grid<float> mKernelNonSeedYear; ///< species specific "seed kernel" (small) for non-seed-years
    Grid<float> mKernelSerotiny; ///< seed kernel for extra seed rain
    Grid<float> mSeedMapSerotiy; ///< seed map that keeps track of serotiny events
    bool mHasPendingSerotiny; ///< true if active (unprocessed) pixels are on the extra-serotiny map
    bool mSetup;
    Species *mSpecies;
    bool mDumpSeedMaps; ///< if true, seedmaps are stored as images
    bool mHasExternalSeedInput; ///< if true, external seeds are modelled for the species
    QString mDumpNextYearFileName; ///< debug output - dump of the content of the grid to a file during the next execution
    int mExternalSeedDirection; ///< direction of external seeds
    int mExternalSeedBuffer; ///< how many 20m pixels away from the simulation area should the seeding start?
    double mExternalSeedBackgroundInput; ///< background propability for this species; if set, then a certain seed availability is provided for the full area
    // external seeds
    Grid<float> mExternalSeedMap; ///< for more complex external seed input, this map holds that information
    void setupExternalSeedsForSpecies(Species *species); ///< setup of special external seed input
    static Grid<float> *mExternalSeedBaseMap; ///< static intermediate data while setting up external seeds
    static QHash<QString, QVector<double> > mExtSeedData; ///< holds definition of species and percentages for external seed input
    static int mExtSeedSizeX, mExtSeedSizeY; ///< size of the sectors used to specify external seed input
};

#endif // SEEDDISPERSAL_H
