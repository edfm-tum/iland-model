#ifndef MODEL_H
#define MODEL_H
#include <QtCore>
#include <QtXml>

#include "global.h"

#include "grid.h"


class RessourceUnit;
class SpeciesSet;

class Model
{
public:
    Model();
    ~Model();
    // start/stop/run
    void beforeRun(); ///< initializations
    void runYear(); ///< run a single year
    void afterStop(); ///< finish and cleanup
    // access to elements
    RessourceUnit *ru() { return mRU.front(); }
    RessourceUnit *ru(QPointF &coord); ///< ressource unit at given coordinates
    // global grids
    FloatGrid *grid() { return mGrid; }
    FloatGrid *heightGrid() { return mHeightGrid; }
    const Grid<RessourceUnit*> &RUgrid() { return mRUmap; }

    // setup/maintenance
    void clear(); ///< free ressources
    void loadProject(); ///< setup and load a project

private:
    void initialize(); ///< basic startup without creating a simulation
    void setupSpace(); ///< setup the "world"(spatial grids, ...), create ressource units

    void applyPattern();
    void readPattern();
    void grow();

    /// container holding all ressource units
    QList<RessourceUnit*> mRU;
    /// grid specifying a map of RessourceUnits
    Grid<RessourceUnit*> mRUmap;
    /// container holding all species sets
    QList<SpeciesSet*> mSpeciesSets;
    // global grids...
    FloatGrid *mGrid;
    FloatGrid *mHeightGrid;
    QList<FloatGrid*> mGridList;
};

#endif // MODEL_H
