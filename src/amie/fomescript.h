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
    Q_PROPERTY(int standId READ standId WRITE setStandId)
public:
    explicit FomeScript(QObject *parent = 0);
    ~FomeScript();
    // prepare scripting features
    void setupScriptEnvironment();
    // functions
    /// prepares the context for executing javascript functions
    /// by setting up all internal structures for the forest stand 'stand'.
    static void setExecutionContext(FMStand *stand);

    /// special function for setting context without a valid stand
    static void setActivity(Activity *act);

    /// static accessor function for the responsible script bridge
    static FomeScript *bridge();
    /// returns a string for debug/trace messages
    const QString &context() const { return mStand?mStand->context():mInvalidContext; }


    StandObj *standObj() const { return mStandObj; }
    SiteObj *siteObj() const { return mSiteObj; }
    FMTreeList *treesObj() const { return mTrees; }
    ActivityObj *activityObj() const { return mActivityObj; }

    // Properties
    /// verbose: when true, the logging intensity is increased significantly.
    bool verbose() const;
    void setVerbose(bool arg);

    int standId() const;
    void setStandId(int new_stand_id);

signals:

public slots:
    /// logging function (which includes exeuction context)
    void log(QJSValue value);
    /// abort execution
    void abort(QJSValue message);
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
    ActivityObj *mActivityObj;
    FMTreeList *mTrees;
    QString mLastErrorMessage;

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
    /// basal area of a given species (m2/ha) given by Id.
    double basalAreaOf(QString species_id) const {return mStand->basalArea(species_id); }
    double basalArea(int index) const { if (index>=0 && index<nspecies()) return mStand->speciesData(index).basalArea; else return 0.; }
    double relBasalArea(int index) const { if (index>=0 && index<nspecies()) return mStand->speciesData(index).relBasalArea; else return 0.; }
    QString speciesId(int index) const;

    // set and get standspecific data (persistent!)
    void setFlag(const QString &name, QJSValue value){ const_cast<FMStand*>(mStand)->setProperty(name, value);}
    QJSValue flag(const QString &name) { return const_cast<FMStand*>(mStand)->property(name); }
    QJSValue activity(QString name);

    // actions
    /// force a reload of the stand data.
    void reload() { mStand->reload(); }

public:
    explicit StandObj(QObject *parent = 0): QObject(parent), mStand(0) {}
    // system stuff
    void setStand(FMStand* stand) { mStand = stand; }
    bool trace() const;
    void setTrace(bool do_trace);

    // properties of the forest
    double basalArea() const { if (mStand)return mStand->basalArea(); throwError("basalArea"); return -1.;}
    double age() const {if (mStand)return mStand->age(); throwError("age"); return -1.;}
    double volume() const {if (mStand) return mStand->volume(); throwError("volume"); return -1.; }
    int id() const { if (mStand) return mStand->id(); throwError("id"); return -1; }
    int nspecies() const {if (mStand) return mStand->nspecies();  throwError("id"); return -1;}


private:
    void throwError(QString msg) const;
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
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool finalHarvest READ finalHarvest WRITE setFinalHarvest)
    Q_PROPERTY(bool scheduled READ scheduled WRITE setScheduled)
    Q_PROPERTY(QString name READ name)
public:
    explicit ActivityObj(QObject *parent = 0): QObject(parent) { mActivityIndex=-1; mStand=0; mActivity=0; }
    // used to construct a link to a given activty (with an index that could be not the currently active index!)
    ActivityObj(FMStand *stand, Activity *act, int index ): QObject(0) { mActivityIndex=index; mStand=stand; mActivity=act; }
    /// default-case: set a forest stand as the context.
    void setStand(FMStand *stand) { mStand = stand; mActivity=0; mActivityIndex=-1;}
    /// set an activity context (without a stand) to access base properties of activities
    void setActivity(Activity *act) { mStand = 0; mActivity=act; mActivityIndex=-1;}
    /// set an activity that is not the current activity of the stand
    void setActivityIndex(const int index) { mActivityIndex = index; }

    // properties

    QString name() const;
    bool enabled() const;
    void setEnabled(bool do_enable);

    bool active() const { return flags().active(); }
    void setActive(bool activate) { flags().setActive(activate);}

    bool finalHarvest() const { return flags().isFinalHarvest(); }
    void setFinalHarvest(bool isfinal) { flags().setFinalHarvest(isfinal);}

    bool scheduled() const { return flags().isScheduled(); }
    void setScheduled(bool issched) { flags().setIsScheduled(issched);}
public slots:
private:
    ActivityFlags &flags() const; // get (depending on the linked objects) the right flags
    int mActivityIndex; // link to base activity
    Activity *mActivity; // pointer
    FMStand *mStand; // and to the forest stand....

};


} // namespace

#endif // FOMESCRIPT_H
