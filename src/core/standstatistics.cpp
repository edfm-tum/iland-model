/** @class StandStatistics collects information on stand level for each tree species.
  Call clear() to clear the statistics, then call add() for each tree and finally calculate().
  To aggregate on a higher level, use add() for each StandStatistics object to include, and then
  calculate() on the higher level.
  */
#include "standstatistics.h"
#include "tree.h"


void StandStatistics::clear()
{
    // reset all values
    mCount = 0;
    mSumDbh=mSumHeight = mAverageDbh=mAverageHeight =0.;
    mSumBasalArea = mSumVolume = 0.;
}

void StandStatistics::add(Tree *tree)
{
    mCount++;
    mSumDbh+=tree->dbh();
    mSumHeight+=tree->height();
    mSumBasalArea+=tree->basalArea();
    mSumVolume += tree->volume();
}

void StandStatistics::calculate()
{
    double dcount = (double) mCount;
    if (mCount) {
        mAverageDbh = mSumDbh / dcount;
        mAverageHeight = mSumHeight / dcount;
    }
}

void StandStatistics::add(const StandStatistics &stat)
{
    mCount+=stat.mCount;
    mSumBasalArea+=stat.mSumBasalArea;
    mSumDbh+=stat.mSumDbh;
    mSumHeight+=stat.mSumHeight;
    mSumVolume+=stat.mSumVolume;
}
