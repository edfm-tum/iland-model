#ifndef STANDSTATISTICS_H
#define STANDSTATISTICS_H
class Tree;
struct TreeGrowthData;
class ResourceUnitSpecies;
class Sapling;

class StandStatistics
{
public:
    StandStatistics() { mRUS=0; clear();}
    void setResourceUnitSpecies(const ResourceUnitSpecies *rus) { mRUS=rus; }

    void add(const StandStatistics &stat); ///< add aggregates of @p stat to own aggregates
    void add(const Tree *tree, const TreeGrowthData *tgd); ///< call for each tree within the domain
    void add(const Sapling *sapling); ///< call for regeneration layer of a species in resource unit
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
    int cohortCount() const { return mCohortCount; } ///< number of cohorts of saplings / ha
    int saplingCount() const { return mSaplingCount; } ///< number individuals in regeneration layer (represented by "cohortCount" cohorts) N/ha
    double saplingAge() const { return mAverageSaplingAge; } ///< average age of sapling (currenty not weighted with represented sapling numbers...)
    // carbon/nitrogen cycle
    double cStem() const { return mCStem; }
    double nStem() const { return mNStem; }
    double cBranch() const { return mCBranch; }
    double nBranch() const { return mNBranch; }
    double cFoliage() const { return mCFoliage; }
    double nFoliage() const { return mNFoliage; }
    double cCoarseRoot() const { return mCCoarseRoot; }
    double nCoarseRoot() const { return mNCoarseRoot; }
    double cFineRoot() const { return mCFineRoot; }
    double nFineRoot() const { return mNFineRoot; }
    double cRegeneration() const { return mCRegeneration; }
    double nRegeneration() const { return mNRegeneration; }

private:
    inline void addBiomass(const double biomass, const double CNRatio, double *C, double *N);
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
    // regeneration layer
    int mCohortCount; ///< number of cohrots
    int mSaplingCount; ///< number of sapling (Reinekes Law)
    double mSumSaplingAge;
    double mAverageSaplingAge;
    // carbon and nitrogen pools
    double mCStem, mCFoliage, mCBranch, mCCoarseRoot, mCFineRoot;
    double mNStem, mNFoliage, mNBranch, mNCoarseRoot, mNFineRoot;
    double mCRegeneration, mNRegeneration;
};


/** holds a couple of system statistics primarily aimed for performance and memory analyis.
  */
class SystemStatistics
{
public:
    SystemStatistics() { reset(); }
    void reset() { treeCount=0; saplingCount=0; newSaplings=0;
                   tManagement = 0.; tApplyPattern=tReadPattern=tTreeGrowth=0.;
                   tSeedDistribution=tEstablishment=tSaplingGrowth=tCarbonCycle=tWriteOutput=tTotalYear=0.; }
    void writeOutput();
    // the system counters
    int treeCount;
    int saplingCount;
    int newSaplings;
    // timings
    double tManagement;
    double tApplyPattern;
    double tReadPattern;
    double tTreeGrowth;
    double tSeedDistribution;
    double tEstablishment;
    double tSaplingGrowth;
    double tCarbonCycle;
    double tWriteOutput;
    double tTotalYear;

};

#endif // STANDSTATISTICS_H
