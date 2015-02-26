#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

#include "grid.h"
#include "layeredgrid.h"
#include "random.h"

#include "bbgenerations.h"

class BarkBeetleCell
{
public:
    BarkBeetleCell() { clear(); }
    void clear() { n=0; dbh=0.f; }
    bool isHost() { return dbh>0.; }
    float dbh; // the dbh of the biggest spruce on the pixel
    int n; // number of cohorts that landed on the pixel
    int killed; // year

};
class BarkBeetleRUCell
{
public:
    BarkBeetleRUCell(): generations(0.), scanned(false) {}
    bool scanned;
    double generations;
};

/** Helper class manage and visualize data layers related to the barkbeetle module.
  @ingroup barkbeetle
*/
class BarkBeetleLayers: public LayeredGrid<BarkBeetleCell> {
  public:
    void setGrid(const Grid<BarkBeetleCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> names() const;
    bool onClick(const QPointF &world_coord) const;
};
class BarkBeetleRULayers: public LayeredGrid<BarkBeetleRUCell> {
  public:
    void setGrid(const Grid<BarkBeetleRUCell> &grid) { mGrid = &grid; }
    double value(const BarkBeetleRUCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> names() const;
    bool onClick(const QPointF &world_coord) const;
};



class ResourceUnit; // forward
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

    /// main function to execute the bark beetle module
    void run();

    void yearBegin();
private:
    void scanResourceUnitTrees(const QPoint &position);
    struct SBBParams {
        SBBParams(): minDbh(10.f), cohortsPerGeneration(30), cohortsPerSisterbrood(50), backgroundInfestationProbability(0.0001) {}
        float minDbh; ///< minimum dbh of spruce trees that are considered as potential hosts
        int cohortsPerGeneration; ///< 'packages' of beetles that spread from an infested pixel
        int cohortsPerSisterbrood; ///< cohorts that spread from a pixel when a full sister brood developed
        QString spreadKernelFormula; ///< formula of the PDF for the BB-spread
        double backgroundInfestationProbability; ///< p that a pixel gets spontaneously infested each year
    } params;



    BBGenerations mGenerations;
    RandomCustomPDF mKernelPDF;
    Grid<BarkBeetleCell> mGrid;
    Grid<BarkBeetleRUCell> mRUGrid;
    BarkBeetleLayers mLayers;
    BarkBeetleRULayers mRULayers;

    friend class BarkBeetleScript;

};



#endif // BARKBEETLEMODULE_H
