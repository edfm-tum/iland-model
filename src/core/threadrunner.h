#ifndef THREADRUNNER_H
#define THREADRUNNER_H

class ResourceUnit;

class ThreadRunner
{
public:
    ThreadRunner();
    bool multithreading() const { return mMultithreaded; }
    void setMultithreading(const bool do_multithreading) { mMultithreaded = do_multithreading; }
    void setup(const QList<ResourceUnit*> &ressourceUnitList);
    void run( ResourceUnit* (*funcptr)(ResourceUnit*) );
    void print();
private:
    QList<ResourceUnit*> mMap1, mMap2;
    bool mMultithreaded;
};

#endif // THREADRUNNER_H
