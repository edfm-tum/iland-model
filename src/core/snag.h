#ifndef SNAG_H
#define SNAG_H
#include <QList>
#include <QVariant>
class Tree; // forward
class ResourceUnit; // forward

/** CNPool stores a duple of carbon and nitrogen (kg/ha)
    use addBiomass(biomass, cnratio) to add biomass; use operators (+, +=, *, *=) for simple operations.
*/
struct CNPool
{
    CNPool(): C(0.), N(0.) {}
    static void setCFraction(const double fraction) { biomassCFraction = fraction; } ///< set the global fraction of carbon of biomass
    CNPool(const double c, const double n) {C=c; N=n; }
    bool isEmpty() const { return C==0.; } ///< returns true if pool is empty
    bool isValid() const { return C>=0. && N>=0.; } ///< return true if pool is valid (content of C or N >=0)
    double C; // carbon pool (kg C/ha)
    double N; // nitrogen pool (kg N/ha)
    double CN() { return N>0?C/N:0.; } ///< current CN ratio
    void clear() {C=0.; N=0.; }
    /// add biomass to the pool (kg dry mass/ha); CNratio is used to calculate the N-Content, the global C-Fraction of biomass is used to
    /// calculate the amount of carbon of 'biomass'.
    void addBiomass(const double biomass, const double CNratio) { C+=biomass*biomassCFraction; N+=biomass*biomassCFraction/CNratio; }
    // some simple operators
    void operator+=(const CNPool &s) { C+=s.C; N+=s.N; } ///< add contents of a pool
    void operator*=(const double factor) { C*=factor; N*=factor; } ///< Multiply pool with 'factor'
    const CNPool operator+(const CNPool &p2) const { return CNPool(C+p2.C, N+p2.N); } ///< return the sum of two pools
    const CNPool operator*(const double factor) const { return CNPool(C*factor, N*factor); } ///< return the pool multiplied with 'factor'
private:
    static double biomassCFraction;
};

class Snag
{
public:
    Snag();
    static void setupThresholds(const double lower, const double upper); ///< setup class thresholds, needs to be called only once... (static)
    void setup( const ResourceUnit *ru); ///< initial setup routine.
    void newYear(); ///< to be executed at the beginning of a simulation year. This cleans up the transfer pools.
    void calculateYear(); ///< to be called at the end of the year (after tree growth, harvesting). Calculates flow to the soil.
    // access
    bool isStateEmpty() const { return mTotalSnagCarbon == 0.; }
    bool isEmpty() const { return mLabileFlux.isEmpty() && mRefractoryFlux.isEmpty() && isStateEmpty(); }
    const CNPool &labileFlux() const { return mLabileFlux; } ///< litter flux to the soil (kg/ha)
    const CNPool &refractoryFlux() const {return  mRefractoryFlux; } ///< deadwood flux to the soil (kg/ha)
    double climateFactor() const { return mClimateFactor; } ///< the 're' climate factor to modify decay rates (also used in ICBM/2N model)
    // actions
    /// add for a tree with diameter
    void addTurnoverLitter(const Tree *tree, const double litter_foliage, const double litter_fineroot);
    /// adds the 'tree' to the appropriate Snag pools.
    void addMortality(const Tree* tree);
    /// add residual biomass of 'tree' after harvesting.
    /// remove_(stem, branch, foliage)_pct: percentage of biomass compartment that is removed by the harvest operation.
    /// the harvested biomass is collected.
    void addHarvest(const Tree* tree, const double remove_stem_pct, const double remove_branch_pct, const double remove_foliage_pct );
    QList<QVariant> debugList(); ///< return a debug output
private:
    double calculateClimateFactors(); ///< calculate climate factor 're' for the current year
    double mClimateFactor; ///< current mean climate factor (influenced by temperature and soil water content)
    const ResourceUnit *mRU; ///< link to resource unit
    /// access SWDPool as function of diameter (cm)
    int poolIndex(const float dbh) { if (dbh<mDBHLower) return 0; if (dbh>mDBHHigher) return 2; return 1;}
    CNPool mSWD[3]; ///< standing woody debris pool (0: smallest dimater class, e.g. <10cm, 1: medium, 2: largest class (e.g. >30cm)) kg/ha
    double mNumberOfSnags[3]; ///< number of snags in diameter class
    double mAvgDbh[3]; ///< average diameter in class (cm)
    double mAvgHeight[3]; ///< average height in class (m)
    double mAvgVolume[3]; ///< average volume in class (m3)
    double mTimeSinceDeath[3]; ///< time since death: mass-weighted age of the content of the snag pool
    double mKSW[3]; ///< standing woody debris decay rate (weighted average of species values)
    double mCurrentKSW[3]; ///< swd decay rate (average for trees of the current year)
    double mHalfLife[3]; ///< half-life values (yrs) (averaged)
    CNPool mToSWD[3]; ///< transfer pool; input of the year is collected here (for each size class)
    CNPool mLabileFlux; ///< flux to labile soil pools (kg/ha)
    CNPool mRefractoryFlux; ///< flux to the refractory soil pool (kg/ha)
    CNPool mBranches[5]; ///< pool for branch biomass
    int mBranchCounter; ///< index which of the branch pools should be emptied
    double mTotalSnagCarbon; ///< sum of carbon content in all snag compartments (kg/ha)
    CNPool mTotalIn; ///< total input to the snag state (i.e. mortality/harvest and litter)
    CNPool mSWDtoSoil; ///< total flux from standing dead wood (book-keeping) -> soil (kg/ha)
    CNPool mTotalToAtm; ///< flux to atmosphere (kg/ha)
    CNPool mTotalToExtern; ///< total flux of masses removed from the site (i.e. harvesting) kg/ha
    static double mDBHLower, mDBHHigher; ///< thresholds used to classify to SWD-Pools
    static double mCarbonThreshold[3]; ///< carbon content thresholds that are used to decide if the SWD-pool should be emptied
};

#endif // SNAG_H
