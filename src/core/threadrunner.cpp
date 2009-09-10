/** @class ThreadRunner encapsulates the invokation of multiple threads for paralellized tasks.
  To avoid lost updates during the light influence pattern application, all the ressourceUnits
  are divided in two lists based on the index (even vs. uneven). These (for almost all cases)
  ensures, that no directly neighboring ressourceUnits are processed.
  */

#include "global.h"
#include "threadrunner.h"
#include <QtCore>

ThreadRunner::ThreadRunner()
{
    mMultithreaded = true;
}

void ThreadRunner::print()
{
    qDebug() << "Multithreading enabled: "<< mMultithreaded << "thread count:" << QThread::idealThreadCount();
}


void ThreadRunner::setup(const QList<RessourceUnit*> &ressourceUnitList)
{
    mMap1.clear(); mMap2.clear();
    bool map=true;
    foreach(RessourceUnit *unit, ressourceUnitList) {
        if (map)
            mMap1.append(unit);
        else
            mMap2.append(unit);

        map = !map;
    }

}

/// run a given function for each ressource unit either multithreaded or not.
void ThreadRunner::run( RessourceUnit* (*funcptr)(RessourceUnit*) )
{
    if (mMultithreaded && mMap1.count() > 20) {
        // execute using QtConcurrent for larger amounts of ressource units...
        QtConcurrent::blockingMap(mMap1,funcptr);
        QtConcurrent::blockingMap(mMap2,funcptr);
    } else {
        // execute serialized in main thread
        RessourceUnit *unit;
        foreach(unit, mMap1)
            (*funcptr)(unit);

        foreach(unit, mMap2)
            (*funcptr)(unit);
    }

}
