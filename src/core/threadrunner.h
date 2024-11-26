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

#ifndef THREADRUNNER_H
#define THREADRUNNER_H
#include <QList>
#include <QtConcurrent/QtConcurrent>
class ResourceUnit;
class Species;
class ThreadRunner
{
public:
    enum RunState { Inactive, SingleThreaded, MultiThreaded};
    ThreadRunner();
    ThreadRunner(const QList<Species*> &speciesList) { setup(speciesList);}

    void setup(const QList<ResourceUnit*> &resourceUnitList);
    void setup(const QList<Species*> &speciesList) { mSpeciesMap = speciesList; }
    // access
    bool multithreading() const { return mMultithreaded; }
    void setMultithreading(const bool do_multithreading) { mMultithreaded = do_multithreading; }
    void print(); ///< print useful debug messages
    // actions
    /// run a function for every resource unit (multithreaded)
    void run( void (*funcptr)(ResourceUnit*), const bool forceSingleThreaded=false ) const; ///< execute 'funcptr' for all resource units in parallel
    /// run a function in parallel for every species
    void run( void (*funcptr)(Species*), const bool forceSingleThreaded=false ) const; ///< execute 'funcptr' for set of species in parallel
    /// run over elements of a vector of type T
    template<class T> void run(T* (*funcptr)(T*), const QVector<T*> &container, const bool forceSingleThreaded=false) const;
    template<class T> void run(void (*funcptr)(T&), QVector<T> &container, const bool forceSingleThreaded=false) const;
    /// run over elements of a vector (of ptrs), but using a member function of an object
    template <class T, class Obj>
    void run(void (Obj::*funcptr)(T*), Obj* obj, const QVector<T*>& container, const bool forceSingleThreaded) const;

    /// run over elements of a vector, but using a member function of an object
    template <class T, class Obj>
    void run(void (Obj::*funcptr)(T&), Obj* obj, const QVector<T>& container, const bool forceSingleThreaded) const;

    /// run over elements of a vector, and call a function of the objects in the container
    template <class T>
    void run(void (T::*funcptr)(), QVector<T>& container, const bool forceSingleThreaded) const;


    // run over chunks of a larger array (or grid)
    template<class T> void runGrid(void (*funcptr)(T*, T*), T* begin, T* end, const bool forceSingleThreaded=false, int minsize=10000, int maxchunks=10000) const;

    /// throw an exception that is safe when running
    /// mulitple threads
    void throwError(const QString &message) const;
    bool hasErrors() const { return  !mErrors.isEmpty(); }
    void clearErrors() { mErrors.clear(); }
    const QStringList errors() const { return mErrors; }
    void checkErrors();
private:
    static QStringList mErrors;
    QList<ResourceUnit*> mMap1, mMap2;
    QList<Species*> mSpeciesMap;
    static RunState mState;
    static bool mMultithreaded;
};

template<class T>
void ThreadRunner::runGrid(void (*funcptr)(T *, T*), T *begin, T *end, const bool forceSingleThreaded, int minsize, int maxchunks) const
{
    int length = end - begin; // # of elements
    if (mMultithreaded && length>minsize*3 && forceSingleThreaded==false) {
        // create multiple calls
        mState = MultiThreaded;
        int chunksize = minsize;
        if (length > chunksize*maxchunks) {
            chunksize = length / maxchunks;
        }
        // execute operations
        T* p = begin;
        while (p<end) {
            T* pend = std::min(p+chunksize, end);
            QFuture result = QtConcurrent::run(funcptr, p, pend);
            p = pend;
        }
    } else {
        // run all in one big function call
        mState = SingleThreaded;
        (*funcptr)(begin, end);
    }
    mState = Inactive;
}

// multirunning function
template<class T>
void ThreadRunner::run(T *(*funcptr)(T *), const QVector<T *> &container, const bool forceSingleThreaded) const
{
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded==false) {
        // execute using QtConcurrent for larger amounts of elements
        mState = MultiThreaded;
        QtConcurrent::blockingMap(container,funcptr);
    } else {
        // execute serialized in main thread
        mState = SingleThreaded;
        T *element;
        foreach(element, container)
            (*funcptr)(element);
    }
    mState = Inactive;

}

// multirunning function
template<class T>
void ThreadRunner::run(void (*funcptr)(T &), QVector<T> &container, const bool forceSingleThreaded) const
{
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded==false) {
        // execute using QtConcurrent for larger amounts of elements
        mState = MultiThreaded;
        QtConcurrent::blockingMap(container,funcptr);
    } else {
        // execute serialized in main thread
        mState = SingleThreaded;
        for (int i=0;i<container.size();++i)
            (*funcptr)(container[i]);

    }
    mState = Inactive;
}

// multithreaded run function for a member function
// to loop over a QVector of pointers
template <class T, class Obj>
void ThreadRunner::run(void (Obj::*funcptr)(T*), Obj* obj, const QVector<T*>& container, const bool forceSingleThreaded) const {
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded == false) {
        // execute using QtConcurrent for larger amounts of elements
        mState = MultiThreaded;
        QtConcurrent::blockingMap(container, [obj, funcptr](T* element) {
            (obj->*funcptr)(element);
        });
    }
    else {
        // execute serialized in main thread
        mState = SingleThreaded;
        T* element;
        foreach(element, container) {
            (obj->*funcptr)(element);
        }
    }
    mState = Inactive;
}

// multithreaded run function for a member function
// to loop over a QVector of objects
template <class T, class Obj>
void ThreadRunner::run(void (Obj::*funcptr)(T&), Obj* obj, const QVector<T>& container, const bool forceSingleThreaded) const {
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded == false) {
        // execute using QtConcurrent for larger amounts of elements
        mState = MultiThreaded;
        QtConcurrent::blockingMap(container, [obj, funcptr](T& element) {
            (obj->*funcptr)(element);
        });
    }
    else {
        // execute serialized in main thread
        mState = SingleThreaded;
        T* element;
        foreach(element, container) {
            (obj->*funcptr)(element);
        }
    }
    mState = Inactive;
}

// multithreaded run function for a member function
// to loop over a QVector of objects
template <class T>
void ThreadRunner::run(void (T::*funcptr)(), QVector<T>& container, const bool forceSingleThreaded) const {
    if (mMultithreaded && container.count() > 3 && forceSingleThreaded == false) {
        // execute using QtConcurrent for larger amounts of elements
        mState = MultiThreaded;
        QtConcurrent::blockingMap(container, [funcptr](T& element) {
            (element.*funcptr)();
        });
    }
    else {
        // execute serialized in main thread
        mState = SingleThreaded;
        for (T& element : container)
            (element.*funcptr)();
    }
    mState = Inactive;
}

#endif // THREADRUNNER_H
