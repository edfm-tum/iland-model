/** @class ThreadRunner
  Encapsulates the invokation of multiple threads for paralellized tasks.
  To avoid lost updates during the light influence pattern application, all the resourceUnits
  are divided in two lists based on the index (even vs. uneven). These (for almost all cases)
  ensures, that no directly neighboring resourceUnits are processed.
  */

#include "global.h"
#include "threadrunner.h"
#include <QtCore>
bool ThreadRunner::mMultithreaded = true; // static

ThreadRunner::ThreadRunner()
{
    mMultithreaded = true;
}

void ThreadRunner::print()
{
    qDebug() << "Multithreading enabled: "<< mMultithreaded << "thread count:" << QThread::idealThreadCount();
}


void ThreadRunner::setup(const QList<ResourceUnit*> &resourceUnitList)
{
    mMap1.clear(); mMap2.clear();
    bool map=true;
    foreach(ResourceUnit *unit, resourceUnitList) {
        if (map)
            mMap1.append(unit);
        else
            mMap2.append(unit);

        map = !map;
    }

}

/// run a given function for each ressource unit either multithreaded or not.
void ThreadRunner::run( ResourceUnit* (*funcptr)(ResourceUnit*), const bool forceSingleThreaded )
{
    if (mMultithreaded && mMap1.count() > 3 && forceSingleThreaded==false) {
        // execute using QtConcurrent for larger amounts of ressource units...
        QtConcurrent::blockingMap(mMap1,funcptr);
        QtConcurrent::blockingMap(mMap2,funcptr);
    } else {
        // execute serialized in main thread
        ResourceUnit *unit;
        foreach(unit, mMap1)
            (*funcptr)(unit);

        foreach(unit, mMap2)
            (*funcptr)(unit);
    }

}

/// run a given function for each species
void ThreadRunner::run( Species* (*funcptr)(Species*), const bool forceSingleThreaded )
{
    if (mMultithreaded && mSpeciesMap.count() > 3 && forceSingleThreaded==false) {
        QtConcurrent::blockingMap(mSpeciesMap, funcptr);
    } else {
        // single threaded operation
        Species *species;
        foreach(species, mSpeciesMap)
            (*funcptr)(species);
    }

}
