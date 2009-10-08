#ifndef STANDSTATISTICS_H
#define STANDSTATISTICS_H
class Tree;
class ResourceUnitSpecies;

class StandStatistics
{
public:
    StandStatistics() { mRUS=0; clear();}
    void setResourceUnitSpecies(const ResourceUnitSpecies *rus) { mRUS=rus; }

    void add(const StandStatistics &stat); ///< add aggregates of @p stat to own aggregates
    void add(const Tree *tree); ///< call for each tree within the domain
    void clear(); ///< call before trees are aggregated
    void calculate(); ///< call after all trees are processed (postprocessing)
    // getters
    int count() const { return mCount; }
    double dbh_avg() const { return mAverageDbh; }
    double height_avg() const { return mAverageHeight; }
    double volume() const { return mSumVolume; }
    double basalArea() const { return mSumBasalArea; }
    double leafAreaIndex() const { return mLeafAreaIndex; }

private:
    const ResourceUnitSpecies *mRUS; ///< link to the resource unit species
    int mCount;
    double mSumDbh;
    double mSumHeight;
    double mSumBasalArea;
    double mSumVolume;
    double mAverageDbh;
    double mAverageHeight;
    double mLeafAreaIndex;
};

#endif // STANDSTATISTICS_H
