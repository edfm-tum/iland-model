#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

#include "grid.h"
#include "layeredgrid.h"
#include "random.h"
#include "expression.h"

#include "bbgenerations.h"

class BarkBeetleCell
{
public:
    BarkBeetleCell() { reset(); }
    void clear() { n=0; killed=0; infested=false;  p_colonize=0.f; }
    void reset() {clear(); dbh=0.f; tree_stress=0.f; }
    bool isHost() const { return dbh>0.f; }
    bool isPotentialHost() const {return dbh>0.f && killed==0 && infested==false; }
    void setInfested(bool is_infested) { infested=is_infested; if (infested) { total_infested++; killed=0; n=0;} }
    void finishedSpread(int iteration) { infested=false; killed=iteration; }
    bool infested;
    float dbh; // the dbh of the biggest spruce on the pixel
    float tree_stress; // the stress rating of this tree
    float p_colonize; // the highest probability (0..1) that a pixel is killed
    int n; // number of cohorts that landed on the pixel
    int killed; // year at which pixel was killed ??
    static void resetCounters() { total_infested=0; }
    static int total_infested;

};
class BarkBeetleRUCell
{
public:
    BarkBeetleRUCell(): generations(0.), scanned(false), add_sister(false), cold_days(0), cold_days_late(0) {}
    bool scanned;
    double generations;
    bool add_sister;
    int cold_days; // number of days in the winter season with t_min below a given threshold (-15 degree Celsius)
    int cold_days_late;
};

/** Helper class manage and visualize data layers related to the barkbeetle module.
  @ingroup barkbeetle
*/
class BarkBeetleLayers: public LayeredGrid<BarkBeetleCell> {
  public:
    void setGrid(const Grid<BarkBeetleCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
};

class BarkBeetleRULayers: public LayeredGrid<BarkBeetleRUCell> {
  public:
    void setGrid(const Grid<BarkBeetleRUCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleRUCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
};



class ResourceUnit; // forward
class BarkBeetleOut; // forward
/** Main class of the bark beetle module.
  @ingroup barkbeetle
*/
class BarkBeetleModule
{
public:
    BarkBeetleModule();
    static double cellsize() { return 10.; }

    void setup(); ///< general setup
    void setup(const ResourceUnit *ru); ///< setup for a specific resource unit
    void loadParameters(); ///< load params from XML
    void clearGrids(); ///< reset the state of the internal grids (used for javascript based tests)

    /// main function to execute the bark beetle module
    void run(int iteration=0);

    void yearBegin();
private:
    void calculateGenerations();
    void startSpread(); ///< beginning of a calculation
    void barkbeetleSpread(); ///< main function of bark beetle spread
    void scanResourceUnitTrees(const QPoint &position);
    int mIteration;
    QString mAfterExecEvent;
    struct SBBParams {
        SBBParams(): minDbh(10.f), cohortsPerGeneration(30), cohortsPerSisterbrood(50), spreadKernelMaxDistance(100.), backgroundInfestationProbability(0.0001) {}
        float minDbh; ///< minimum dbh of spruce trees that are considered as potential hosts
        int cohortsPerGeneration; ///< 'packages' of beetles that spread from an infested pixel
        int cohortsPerSisterbrood; ///< cohorts that spread from a pixel when a full sister brood developed
        QString spreadKernelFormula; ///< formula of the PDF for the BB-spread
        double spreadKernelMaxDistance; ///< upper limit for the spread distance (the kernel is cut at this distance)
        double backgroundInfestationProbability; ///< p that a pixel gets spontaneously infested each year
        double winterMortalityBaseLevel; ///< p that a infested pixel dies out over the winter (due to antagonists, bad luck, ...)
    } params;
    struct SBBStats {
        void clear() { infestedStart=0;infestedBackground=0; maxGenerations=0;NCohortsLanded=0;NPixelsLanded=0;NCohortsSpread=0;NInfested=0;NWinterMortality=0;}
        int infestedStart; // # of pixels that are infested at the beginning of an iteration
        int infestedBackground; // # of pixels that are getting active
        int maxGenerations; // maxium number of generations found this year
        int NCohortsLanded; // number of cohorts that landed on new potential host pixels
        int NPixelsLanded; // number of potential host pixels that received at least one cohort
        int NCohortsSpread; // number of pixels that are spread from infested cells
        int NInfested; // number of newly infested pixels (those of the 'landed'
        int NWinterMortality; // number of (infested) pixels that died off during winter
    } stats;



    BBGenerations mGenerations;
    RandomCustomPDF mKernelPDF;
    Expression mColonizeProbability;
    Expression mWinterMortalityFormula; ///< temperature dependent winter mortality (more beetle die if there are more cold days)
    Grid<BarkBeetleCell> mGrid;
    Grid<BarkBeetleRUCell> mRUGrid;
    BarkBeetleLayers mLayers;
    BarkBeetleRULayers mRULayers;

    friend class BarkBeetleScript;
    friend class BarkBeetleOut;

};



#endif // BARKBEETLEMODULE_H
