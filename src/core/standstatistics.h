#ifndef STANDSTATISTICS_H
#define STANDSTATISTICS_H
class Tree;
struct TreeGrowthData;
class ResourceUnitSpecies;

class StandStatistics
{
public:
    StandStatistics() { mRUS=0; clear();}
    void setResourceUnitSpecies(const ResourceUnitSpecies *rus) { mRUS=rus; }

    void add(const StandStatistics &stat); ///< add aggregates of @p stat to own aggregates
    void add(const Tree *tree, const TreeGrowthData *tgd); ///< call for each tree within the domain
    void clear(); ///< call before trees are aggregated
    void calculate(); ///< call after all trees are processed (postprocessing)
    // getters
    int count() const { return mCount; }
    double dbh_avg() const { return mAverageDbh; } ///< average dbh (cm)
    double height_avg() const { return mAverageHeight; } ///< average tree height (m)
    double volume() const { return mSumVolume; } ///< sum of tree volume (m3/ha)
    double gwl() const { return mGWL;} ///< total increment (m3/ha)
    double basalArea() const { return mSumBasalArea; } ///< sum of basal area of all trees (m2/ha)
    double leafAreaIndex() const { return mLeafAreaIndex; } ///< [m2/m2]/ha stocked area.
    double npp() const { return mNPP; } ///< sum. of NPP (kg Biomass increment, above+belowground)/ha
    double nppAbove() const { return mNPPabove; } ///< above ground NPP (kg Biomass increment)/ha

private:
    const ResourceUnitSpecies *mRUS; ///< link to the resource unit species
    int mCount;
    double mSumDbh;
    double mSumHeight;
    double mSumBasalArea;
    double mSumVolume;
    double mGWL;
    double mAverageDbh;
    double mAverageHeight;
    double mLeafAreaIndex;
    double mNPP;
    double mNPPabove;
};

#endif // STANDSTATISTICS_H
