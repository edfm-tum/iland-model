#ifndef FOMESCRIPT_H
#define FOMESCRIPT_H

#include <QObject>

#include "fmstand.h"
#include "fmunit.h"
namespace AMIE {

class StandObj;
class SiteObj;
class SimulationObj;
class FMTreeList; // forward

/// FomeScript provides general helping functions for the Javascript world.
class FomeScript : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose)
public:
    explicit FomeScript(QObject *parent = 0);
    ~FomeScript();
    // prepare scripting features
    void setupScriptEnvironment();
    // functions
    /// prepares the context for executing javascript functions
    /// by setting up all internal structures for the forest stand 'stand'.
    static void setExecutionContext(FMStand *stand);

    /// static accessor function for the responsible script bridge
    static FomeScript *bridge();
    /// returns a string for debug/trace messages
    const QString &context() const { return mStand?mStand->context():mInvalidContext; }


    StandObj *standObj() const { return mStandObj; }
    SiteObj *siteObj() const { return mSiteObj; }
    FMTreeList *treesObj() const { return mTrees; }

    // Properties
    /// verbose: when true, the logging intensity is increased significantly.
    bool verbose() const;
    void setVerbose(bool arg);

signals:

public slots:
    /// logging function (which includes exeuction context)
    void log(QJSValue value);
    /// adds a management program (STP) that is provided as the Javascript object 'program'. 'name' is used internally.
    bool addManagement(QJSValue program, QString name);
    /// add an agent definition (Javascript). 'name' is used internally. Returns true on success.
    bool addAgent(QJSValue program, QString name);
    /// executes an activity for stand 'stand_id'. This bypasses the normal scheduling (useful for debugging/testing).
    /// returns false if activity could not be found for the stand.
    bool runActivity(int stand_id, QString activity);


private:
    static QString mInvalidContext;
    const FMStand *mStand;
    StandObj *mStandObj;
    SiteObj *mSiteObj;
    SimulationObj *mSimulationObj;
    FMTreeList *mTrees;
    bool m_verbose;
};
class ActivityObj;

/// StandObj is the bridge to stand variables from the Javascript world
class StandObj: public QObject
{
    Q_OBJECT
    Q_PROPERTY (bool trace READ trace WRITE setTrace)
    Q_PROPERTY (double basalArea READ basalArea)
    Q_PROPERTY (double age READ age)
    Q_PROPERTY (double volume READ volume)
    Q_PROPERTY (int id READ id)
    Q_PROPERTY (int nspecies READ nspecies)
/*    basalArea: 0, // total basal area/ha of the stand
    volume: 100, // total volume/ha of the stand
    speciesCount: 3, // number of species present in the stand with trees > 4m
    age: 100, // "age" of the stand (in relation to "U")
    flags: {}*/
public slots:
    /// basal area of a given species (m2/ha)
    double basalArea(QString species_id) const {return mStand->basalArea(species_id); }

    // set and get standspecific data (persistent!)
    void setFlag(const QString &name, QJSValue value){ const_cast<FMStand*>(mStand)->setProperty(name, value);}
    QJSValue flag(const QString &name) { return const_cast<FMStand*>(mStand)->property(name); }
    QJSValue activity(QString name);

public:
    explicit StandObj(QObject *parent = 0): QObject(parent), mStand(0) {}
    // system stuff
    void setStand(FMStand* stand) { mStand = stand; }
    bool trace() const;
    void setTrace(bool do_trace);

    // properties of the forest
    double basalArea() const { return mStand->basalArea(); }
    double age() const {return mStand->age(); }
    double volume() const {return mStand->volume(); }
    int id() const { return mStand->id(); }
    int nspecies() const { return mStand->nspecies(); }


private:
    FMStand *mStand;
};


class SiteObj: public QObject
{
    Q_OBJECT
    Q_PROPERTY (QString harvestMode READ harvestMode)
public:
    explicit SiteObj(QObject *parent = 0): QObject(parent) {}
    void setStand(const FMStand* stand) { mStand = stand; }
    QString harvestMode() const { return "schlepper"; } // dummy
private:
    const FMStand *mStand;

};

class SimulationObj: public QObject
{
    Q_OBJECT
    Q_PROPERTY (double timberPriceIndex READ timberPriceIndex)
public:
    explicit SimulationObj(QObject *parent = 0): QObject(parent) {}
    double timberPriceIndex() const { return 1.010101; } // dummy
private:

};


class ActivityObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY (bool enabled READ enabled WRITE setEnabled)
    Q_PROPERTY(QString name READ name)
public:
    bool enabled() const;
    QString name() const;
    void setEnabled(bool do_enable);
    explicit ActivityObj(QObject *parent = 0): QObject(parent) { mActivityIndex=-1; mStand=0; }
    ActivityObj(FMStand *stand, Activity *act, int index ): QObject(0) { mActivityIndex=index; mStand=stand; mActivity=act; }
public slots:
private:
    int mActivityIndex; // link to base activity
    Activity *mActivity; // pointer
    FMStand *mStand; // and to the forest stand....

};


} // namespace

#endif // FOMESCRIPT_H
