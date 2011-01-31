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
class TimeEvents;
class MapGrid;

struct HeightGridValue
{
    float height;
    int count() const { return mCount & 0x0000ffff; } ///< get count of trees on pixel
    void increaseCount() { mCount++; } ///< increase the number of trees on pixel
    void resetCount() { mCount &= 0xffff0000; } ///< set the count to 0
    bool isValid() const { return !isBitSet(mCount, 16); } ///< a value of 1: not valid (returns false)
    void setValid(const bool valid) { setBit(mCount, 16, !valid); } ///< set bit to 1: pixel is not valid
    void init(const float aheight, const int acount) { height=aheight;mCount=acount; }
private:

    int mCount; // the lower 16 bits are to count, the heigher for flags. bit 16: valid (0=valid, 1=outside of project area)

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
    const QRectF &extent() const { return mModelRect; } ///< extent of the model (without buffer)
    ResourceUnit *ru() { return mRU.front(); }
    ResourceUnit *ru(QPointF coord); ///< ressource unit at given coordinates
    ResourceUnit *ru(int index) { return (index>=0&&index<mRU.count())? mRU[index] : NULL; } ///< get resource unit by index
    const QList<ResourceUnit*> &ruList() const {return mRU; }
    Management *management() const { return mManagement; }
    Environment *environment() {return mEnvironment; }
    SpeciesSet *speciesSet() const { if (mSpeciesSets.count()==1) return mSpeciesSets.first(); return NULL; }
    // global grids
    FloatGrid *grid() { return mGrid; } ///< this is the global 'LIF'-grid (light patterns) (currently 2x2m)
    HeightGrid *heightGrid() { return mHeightGrid; } ///< stores maximum heights of trees and some flags (currently 10x10m)
    const MapGrid *standGrid() { return mStandGrid; } ///< retrieve the spatial grid that defines the stands (10m resolution)
    const Grid<ResourceUnit*> &RUgrid() { return mRUmap; }

    // setup/maintenance
    void clear(); ///< free ressources
    void loadProject(); ///< setup and load a project
    bool isSetup() const { return mSetup; } ///< return true if the model world is correctly setup.
    static const ModelSettings &settings() {return mSettings;} ///< access to global model settings.
    static ModelSettings &changeSettings() {return mSettings;} ///< write access to global model settings.
    void onlyApplyLightPattern() { applyPattern(); readPattern(); }
    // actions
    void createStandStatistics();
    /// execute a function for each resource unit using multiple threads. "funcptr" is a ptr to a simple function
    void executePerResourceUnit(ResourceUnit * (*funcptr)(ResourceUnit*), const bool forceSingleThreaded=false) { threadRunner.run(funcptr, forceSingleThreaded);}

private:
    void initialize(); ///< basic startup without creating a simulation
    void setupSpace(); ///< setup the "world"(spatial grids, ...), create ressource units
    void initOutputDatabase(); ///< setup output database (run metadata, ...)


    void applyPattern(); ///< apply LIP-patterns of all trees
    void readPattern(); ///< retrieve LRI for trees
    void grow(); ///< grow - both on RU-level and tree-level

    void calculateStockedArea(); ///< calculate area stocked with trees for each RU
    void calculateStockableArea(); ///< calculate the stockable area for each RU (i.e.: with stand grid values <> -1)

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
    //
    QRectF mModelRect; ///< extent of the model (without buffer)
    // global grids...
    FloatGrid *mGrid; ///< the main LIF grid of the model (2x2m resolution)
    HeightGrid *mHeightGrid; ///< grid with 10m resolution that stores maximum-heights, tree counts and some flags
    Management *mManagement; ///< management sub-module
    Environment *mEnvironment; ///< definition of paramter values on resource unit level (modify the settings tree)
    TimeEvents *mTimeEvents; ///< sub module to handle predefined events in time (modifies the settings tree in time)
    MapGrid *mStandGrid; ///< map of the stand map (10m resolution)
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
