#ifndef MODEL_H
#define MODEL_H
#include <QtCore>
#include <QtXml>

#include "global.h"

#include "grid.h"
#include "threadrunner.h"
#include "modelsettings.h"

class ResourceUnit;
class SpeciesSet;
class Management;
class Climate;
class Environment;

struct HeightGridValue
{
    float height;
    int count;
};
typedef Grid<HeightGridValue> HeightGrid;

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
    ResourceUnit *ru() { return mRU.front(); }
    ResourceUnit *ru(QPointF &coord); ///< ressource unit at given coordinates
    const QList<ResourceUnit*> &ruList() const {return mRU; }
    Management *management() const { return mManagement; }
    // global grids
    FloatGrid *grid() { return mGrid; }
    HeightGrid *heightGrid() { return mHeightGrid; }
    const Grid<ResourceUnit*> &RUgrid() { return mRUmap; }

    // setup/maintenance
    void clear(); ///< free ressources
    void loadProject(); ///< setup and load a project
    bool isSetup() const { return mSetup; } ///< return true if the model world is correctly setup.
    static const ModelSettings &settings() {return mSettings;} ///< access to global model settings.
    static ModelSettings &changeSettings() {return mSettings;} ///< write access to global model settings.

    // actions
    void createStandStatistics();

private:
    void initialize(); ///< basic startup without creating a simulation
    void setupSpace(); ///< setup the "world"(spatial grids, ...), create ressource units


    void applyPattern(); ///< apply LIP-patterns of all trees
    void readPattern(); ///< retrieve LRI for trees
    void grow(); ///< grow - both on RU-level and tree-level

    void calculateStockedArea(); ///< calculate area stocked with trees for each RU

    void test();
    bool multithreading() const { return threadRunner.multithreading(); }
    ThreadRunner threadRunner;
    static ModelSettings mSettings;
    bool mSetup;
    /// container holding all ressource units
    QList<ResourceUnit*> mRU;
    /// grid specifying a map of ResourceUnits
    Grid<ResourceUnit*> mRUmap;
    /// container holding all species sets
    QList<SpeciesSet*> mSpeciesSets;
    /// container holding all the climate objects
    QList<Climate*> mClimates;
    // global grids...
    FloatGrid *mGrid;
    HeightGrid *mHeightGrid;
    Management *mManagement;
    Environment *mEnvironment;
};

class Tree;
class AllTreeIterator
{
public:
    AllTreeIterator(Model* model): mModel(model), mTreeEnd(0),mCurrent(0) {}
    void reset() { mTreeEnd=0; mCurrent=0; }
    Tree *next();
    Tree *nextLiving();
    Tree *current() const;
    Tree *operator*() const { return current();  }
    ResourceUnit *currentRU() const { return *mRUIterator; }
private:
    Model *mModel;
    Tree *mTreeEnd;
    Tree *mCurrent;
    QList<ResourceUnit*>::const_iterator mRUIterator;
};
#endif // MODEL_H
