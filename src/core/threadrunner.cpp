/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

/** @class ThreadRunner
  Encapsulates the invokation of multiple threads for paralellized tasks.
  To avoid lost updates during the light influence pattern application, all the resourceUnits
  are divided in two lists based on the index (even vs. uneven). These (for almost all cases)
  ensures, that no directly neighboring resourceUnits are processed.
  */

#include "global.h"
#include "threadrunner.h"
#include <QtCore>
#include <QtConcurrent/QtConcurrent>
bool ThreadRunner::mMultithreaded = true; // static
QStringList ThreadRunner::mErrors = {};
ThreadRunner::RunState ThreadRunner::mState = Inactive;

ThreadRunner::ThreadRunner()
{
    mMultithreaded = true;
    mState = Inactive;
}

void ThreadRunner::print()
{
    qDebug() << "Multithreading enabled: "<< mMultithreaded << "thread count:" << QThread::idealThreadCount();
}


void ThreadRunner::setup(const QList<ResourceUnit*> &resourceUnitList)
{
    mMap1.clear(); mMap2.clear();
    bool map_to_list1=true;
    foreach(ResourceUnit *unit, resourceUnitList) {
        if (map_to_list1)
            mMap1.append(unit);
        else
            mMap2.append(unit);

        map_to_list1 = !map_to_list1;
    }

}

/// run a given function for each ressource unit either multithreaded or not.
void ThreadRunner::run(void (*funcptr)(ResourceUnit *), const bool forceSingleThreaded ) const
{
    if (mMultithreaded && mMap1.count() > 3 && forceSingleThreaded==false) {
        // execute using QtConcurrent for larger amounts of ressource units...
        mState = MultiThreaded;
        executeBlocking(mMap1,funcptr);
        executeBlocking(mMap2,funcptr);
    } else {
        // execute serialized in main thread
        mState = SingleThreaded;
        ResourceUnit *unit;
        long long counter = 0;
        foreach(unit, mMap1) {
            (*funcptr)(unit);
            DebugTimer::checkResponsiveness(counter++, 100);
        }

        foreach(unit, mMap2) {
            (*funcptr)(unit);
            DebugTimer::checkResponsiveness(counter++, 100);
        }
    }
    mState = Inactive;

}

/// run a given function for each species
void ThreadRunner::run(void (*funcptr)(Species *), const bool forceSingleThreaded ) const
{
    if (mMultithreaded && mSpeciesMap.count() > 3 && forceSingleThreaded==false) {
        mState = MultiThreaded;
        executeBlocking(mSpeciesMap, funcptr);
    } else {
        // single threaded operation
        mState = SingleThreaded;
        Species *species;
        long long counter = 0;
        foreach(species, mSpeciesMap) {
            (*funcptr)(species);
            DebugTimer::checkResponsiveness(counter++, 100);
        }
    }
    mState = Inactive;
}

QMutex _errorMutex;
void ThreadRunner::throwError(const QString &message) const
{
    if (mState == Inactive || mState == SingleThreaded) {
        // we are safe to just throw the error
        throw IException(message);
    } else {
        // the exception was raised while running multiple threads
        // we cannot directly throw the error, but write in an list of errors
        // for later processing
        QMutexLocker locker(&_errorMutex);
        mErrors.append(message);
    }
}

void ThreadRunner::checkErrors()
{
    if (!hasErrors())
        return;
    QString full_message = mErrors.join('\n');
    if (full_message.length() > 1000)
        full_message = full_message.mid(0, 1000) + "...";
    throw IException(QString("Error in multi-threaded code: %1").arg(full_message));
}
