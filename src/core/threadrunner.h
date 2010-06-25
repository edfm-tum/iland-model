#ifndef THREADRUNNER_H
#define THREADRUNNER_H
#include <QList>
class ResourceUnit;

class ThreadRunner
{
public:
    ThreadRunner();
    bool multithreading() const { return mMultithreaded; }
    void setMultithreading(const bool do_multithreading) { mMultithreaded = do_multithreading; }
    void setup(const QList<ResourceUnit*> &resourceUnitList);
    void run( ResourceUnit* (*funcptr)(ResourceUnit*), const bool forceSingleThreaded=false );
    void print();
private:
    QList<ResourceUnit*> mMap1, mMap2;
    bool mMultithreaded;
};

#endif // THREADRUNNER_H
