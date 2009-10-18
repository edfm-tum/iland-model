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


void StandStatistics::clear()
{
    // reset all values
    mCount = 0;
    mSumDbh=mSumHeight = mAverageDbh=mAverageHeight =0.;
    mSumBasalArea = mSumVolume = mGWL = 0.;
    mLeafAreaIndex = 0.;
    mNPP = mNPPabove = 0.;
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
}

// note: mRUS = 0 for aggregated statistics
void StandStatistics::calculate()
{
    double dcount = (double) mCount;
    if (mCount) {
        mAverageDbh = mSumDbh / dcount;
        mAverageHeight = mSumHeight / dcount;
        if (mRUS)
            mLeafAreaIndex /= mRUS->ru()->area(); // convert from leafarea to LAI
    }
    // scale values to per hectare if resource unit <> 1ha
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
}
