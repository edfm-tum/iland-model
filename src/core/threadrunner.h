#ifndef THREADRUNNER_H
#define THREADRUNNER_H

class RessourceUnit;

class ThreadRunner
{
public:
    ThreadRunner();
    bool multithreading() const { return mMultithreaded; }
    void setMultithreading(const bool do_multithreading) { mMultithreaded = do_multithreading; }
    void setup(const QList<RessourceUnit*> &ressourceUnitList);
    void run( RessourceUnit* (*funcptr)(RessourceUnit*) );
    void print();
private:
    QList<RessourceUnit*> mMap1, mMap2;
    bool mMultithreaded;
};

#endif // THREADRUNNER_H
