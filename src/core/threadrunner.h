#ifndef THREADRUNNER_H
#define THREADRUNNER_H
#include <QList>
class ResourceUnit;
class Species;
class ThreadRunner
{
public:
    ThreadRunner();
    ThreadRunner(const QList<Species*> &speciesList) { setup(speciesList);}

    void setup(const QList<ResourceUnit*> &resourceUnitList);
    void setup(const QList<Species*> &speciesList) { mSpeciesMap = speciesList; }
    // access
    bool multithreading() const { return mMultithreaded; }
    void setMultithreading(const bool do_multithreading) { mMultithreaded = do_multithreading; }
    void print(); ///< print useful debug messages
    // actions
    void run( ResourceUnit* (*funcptr)(ResourceUnit*), const bool forceSingleThreaded=false ); ///< execute 'funcptr' for all resource units in parallel
    void run( Species* (*funcptr)(Species*), const bool forceSingleThreaded=false ); ///< execute 'funcptr' for set of species in parallel
private:
    QList<ResourceUnit*> mMap1, mMap2;
    QList<Species*> mSpeciesMap;
    static bool mMultithreaded;
};

#endif // THREADRUNNER_H
