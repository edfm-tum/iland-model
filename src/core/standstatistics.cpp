/** @class StandStatistics
  Collects information on stand level for each tree species.
  Call clear() to clear the statistics, then call add() for each tree and finally calculate().
  To aggregate on a higher level, use add() for each StandStatistics object to include, and then
  calculate() on the higher level.
  Todo-List for new items:
  - add a member variable and a getter
  - add to "add(Tree)" and "calculate()"
  - add to "add(StandStatistics)" as well!
  */
#include "standstatistics.h"
#include "tree.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "sapling.h"
#include "species.h"

void StandStatistics::clear()
{
    // reset all values
    mCount = 0;
    mSumDbh=mSumHeight = mAverageDbh=mAverageHeight =0.;
    mSumBasalArea = mSumVolume = mGWL = 0.;
    mLeafAreaIndex = 0.;
    mNPP = mNPPabove = 0.;
    mCohortCount = mSaplingCount = 0;
    mAverageSaplingAge = 0.;
    mSumSaplingAge = 0.;
    mCStem=0., mCFoliage=0., mCBranch=0., mCCoarseRoot=0., mCFineRoot=0.;
    mNStem=0., mNFoliage=0., mNBranch=0., mNCoarseRoot=0., mNFineRoot=0.;
    mCRegeneration=0., mNRegeneration=0.;

}

void StandStatistics::addBiomass(const double biomass, const double CNRatio, double *C, double *N)
{
    *C+=biomass*biomassCFraction;
    *N+=biomass*biomassCFraction/CNRatio;
}

void StandStatistics::add(const Tree *tree, const TreeGrowthData *tgd)
{
    mCount++;
    mSumDbh+=tree->dbh();
    mSumHeight+=tree->height();
    mSumBasalArea+=tree->basalArea();
    mSumVolume += tree->volume();
    mLeafAreaIndex += tree->leafArea(); // warning: sum of leafarea!
    if (tgd) {
        mNPP += tgd->NPP;
        mNPPabove += tgd->NPP_above;
    }
    // carbon and nitrogen pools
    addBiomass(tree->biomassStem(), tree->species()->cnWood(), &mCStem, &mNStem);
    addBiomass(tree->biomassBranch(), tree->species()->cnWood(), &mCBranch, &mNBranch);
    addBiomass(tree->biomassFoliage(), tree->species()->cnFoliage(), &mCFoliage, &mNFoliage);
    addBiomass(tree->biomassFineRoot(), tree->species()->cnFineroot(), &mCFineRoot, &mNFineRoot);
    addBiomass(tree->biomassCoarseRoot(), tree->species()->cnWood(), &mCCoarseRoot, &mNCoarseRoot);
}

// note: mRUS = 0 for aggregated statistics
void StandStatistics::calculate()
{
    double dcount = (double) mCount;
    if (mCount) {
        mAverageDbh = mSumDbh / dcount;
        mAverageHeight = mSumHeight / dcount;
        if (mRUS && mRUS->ru()->stockableArea()>0.)
            mLeafAreaIndex /= mRUS->ru()->stockableArea(); // convert from leafarea to LAI
    }
    if (mCohortCount)
        mAverageSaplingAge = mSumSaplingAge / double(mCohortCount);

    // scale values to per hectare if resource unit <> 1ha
    // note: no scaling for carbon/nitrogen pools
    if (mRUS) {
        mGWL = mSumVolume + mRUS->removedVolume();
        double area_factor =  10000. / mRUS->ru()->area();
        if (area_factor!=1.) {
            mCount = int(mCount * area_factor);
            mSumBasalArea *= area_factor;
            mSumVolume *= area_factor;
            mNPP *= area_factor;
            mNPPabove *= area_factor;
            mGWL *= area_factor;
            mCohortCount *= area_factor;
            mSaplingCount *= area_factor;
        }
    }
}

void StandStatistics::add(const StandStatistics &stat)
{
    mCount+=stat.mCount;
    mSumBasalArea+=stat.mSumBasalArea;
    mSumDbh+=stat.mSumDbh;
    mSumHeight+=stat.mSumHeight;
    mSumVolume+=stat.mSumVolume;
    mLeafAreaIndex += stat.mLeafAreaIndex;
    mNPP += stat.mNPP;
    mNPPabove += stat.mNPPabove;
    mGWL+=stat.mGWL;
    // regeneration
    mCohortCount += stat.mCohortCount;
    mSaplingCount += stat.mSaplingCount;
    mSumSaplingAge += stat.mSumSaplingAge;
    // carbon/nitrogen pools
    mCStem += stat.mCStem; mNStem += stat.mNStem;
    mCBranch += stat.mCBranch; mNBranch += stat.mNBranch;
    mCFoliage += stat.mCFoliage; mNFoliage += stat.mNFoliage;
    mCFineRoot += stat.mCFineRoot; mNFineRoot += stat.mNFineRoot;
    mCCoarseRoot += stat.mCCoarseRoot; mNCoarseRoot += stat.mNCoarseRoot;
    mCRegeneration += stat.mCRegeneration; mNRegeneration += stat.mNRegeneration;

}

/// call for regeneration layer of a species in resource unit
void StandStatistics::add(const Sapling *sapling)
{
    mCohortCount = sapling->livingSaplings();
    mSaplingCount = sapling->livingSaplings(); // to change!!! Reineke!

    mSumSaplingAge = sapling->averageAge() * sapling->livingSaplings();
    mAverageSaplingAge = sapling->averageAge();

    mCRegeneration += 1.; // todo: fix
    mNRegeneration += 1; // todo: fix
}
