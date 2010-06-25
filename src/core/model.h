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

struct HeightGridValue
{
    float height;
    int count() const { return mCount & 0x0000ffff; } /// get count of trees on pixel
    void increaseCount() { mCount++; } ///< increase the number of trees on pixel
    bool isValid() const { return (mCount >> 16)==0; }
    void setValid(const bool valid) { setBit(mCount, 16, !valid); }
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
    ResourceUnit *ru() { return mRU.front(); }
    ResourceUnit *ru(QPointF &coord); ///< ressource unit at given coordinates
    ResourceUnit *ru(int index) { return (index>=0&&index<mRU.count())? mRU[index] : NULL; } ///< get resource unit by index
    const QList<ResourceUnit*> &ruList() const {return mRU; }
    Management *management() const { return mManagement; }
    Environment *environment() {return mEnvironment; }
    // global grids
    FloatGrid *grid() { return mGrid; } ///< this is the global 'LIF'-grid (light patterns) (currently 2x2m)
    HeightGrid *heightGrid() { return mHeightGrid; } ///< stores maximum heights of trees and some flags (currently 10x10m)
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
    TimeEvents *mTimeEvents;
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
