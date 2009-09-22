#ifndef STANDSTATISTICS_H
#define STANDSTATISTICS_H
class Tree;

class StandStatistics
{
public:
    StandStatistics() { clear();}
    void add(const StandStatistics &stat); ///< add aggregates of @p stat to own aggregates
    void add(Tree *tree); ///< call for each tree within the domain
    void clear(); ///< call before trees are aggregated
    void calculate(); ///< call after tree aggregation is finished
    // getters
    int count() const { return mCount; }
    double dbh_avg() const { return mAverageDbh; }
    double height_avg() const { return mAverageHeight; }
    double volume() const { return mSumVolume; }
    double basalArea() const { return mSumBasalArea; }

private:
    int mCount;
    double mSumDbh;
    double mSumHeight;
    double mSumBasalArea;
    double mSumVolume;
    double mAverageDbh;
    double mAverageHeight;


};

#endif // STANDSTATISTICS_H
