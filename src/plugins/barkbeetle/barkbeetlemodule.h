#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

#include "grid.h"
#include "layeredgrid.h"

#include "bbgenerations.h"

class BarkBeetleCell
{
public:
    BarkBeetleCell(): n(0) {}
    int n;
};
class BarkBeetleRUCell
{
public:
    BarkBeetleRUCell(): generations(0.) {}
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

    /// main function to execute the bark beetle module
    void run();

    void yearBegin();
private:
    BBGenerations mGenerations;
    Grid<BarkBeetleCell> mGrid;
    Grid<BarkBeetleRUCell> mRUGrid;
    BarkBeetleLayers mLayers;
    BarkBeetleRULayers mRULayers;

    friend class BarkBeetleScript;

};



#endif // BARKBEETLEMODULE_H
