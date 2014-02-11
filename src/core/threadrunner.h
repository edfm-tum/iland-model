/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
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

#ifndef THREADRUNNER_H
#define THREADRUNNER_H
#include <QList>
#include <QtConcurrent/QtConcurrent>
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
    template<class T> void run(T* (*funcptr)(T*), const QVector<T*> &container, const bool forceSingleThreaded=false) const;
private:
    QList<ResourceUnit*> mMap1, mMap2;
    QList<Species*> mSpeciesMap;
    static bool mMultithreaded;
};

// multirunning function
template<class T>
void ThreadRunner::run(T *(*funcptr)(T *), const QVector<T *> &container, const bool forceSingleThreaded) const
{
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded==false) {
        // execute using QtConcurrent for larger amounts of elements
        QtConcurrent::blockingMap(container,funcptr);
    } else {
        // execute serialized in main thread
        T *element;
        foreach(element, container)
            (*funcptr)(element);
    }

}

#endif // THREADRUNNER_H
