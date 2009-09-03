#ifndef MODEL_H
#define MODEL_H
#include <QtCore>
#include <QtXml>

#include "global.h"

#include "grid.h"
#include "threadrunner.h"

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
    const QList<RessourceUnit*> &ruList() {return mRU; }
    // global grids
    FloatGrid *grid() { return mGrid; }
    FloatGrid *heightGrid() { return mHeightGrid; }
    const Grid<RessourceUnit*> &RUgrid() { return mRUmap; }

    // setup/maintenance
    void clear(); ///< free ressources
    void loadProject(); ///< setup and load a project
    bool isSetup() const { return mSetup; } ///< return true if the model world is correctly setup.

private:
    void initialize(); ///< basic startup without creating a simulation
    void setupSpace(); ///< setup the "world"(spatial grids, ...), create ressource units

    void applyPattern();
    void readPattern();
    void grow();

    const bool multithreading() const { return threadRunner.multithreading(); }
    ThreadRunner threadRunner;
    bool mSetup;
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

class Tree;
class AllTreeIterator
{
public:
    AllTreeIterator(Model* model): mModel(model), mTreeEnd(0),mCurrent(0) {}
    Tree *next();
    RessourceUnit *currentRU() const { return *mRUIterator; }
private:
    Model *mModel;
    Tree *mTreeEnd;
    Tree *mCurrent;
    QList<RessourceUnit*>::const_iterator mRUIterator;
};
#endif // MODEL_H
