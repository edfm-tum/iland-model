#ifndef FOMESCRIPT_H
#define FOMESCRIPT_H

#include <QObject>

#include "fmstand.h"
#include "fmunit.h"
namespace AMIE {

class StandObj;
class SiteObj;
class SimulationObj;

/// FomeScript provides general helping functions for the Javascript world.
class FomeScript : public QObject
{
    Q_OBJECT
public:
    explicit FomeScript(QObject *parent = 0);
    // prepare scripting features
    void setupScriptEnvironment();
    // functions
    static void setExecutionContext(const FMStand *stand);

signals:

public slots:
    /// adds a management program (STP) that is provided as the Javascript object 'program'. 'name' is used internally.
    bool addManagement(QJSValue program, QString name);
private:
    StandObj *mStandObj;
    SiteObj *mSiteObj;
    SimulationObj *mSimulationObj;

};

/// StandObj is the bridge to stand variables from the Javascript world
class StandObj: public QObject
{
    Q_OBJECT
    Q_PROPERTY (double basalArea READ basalArea)
    Q_PROPERTY (double age READ age)
    Q_PROPERTY (double volume READ volume)
    Q_PROPERTY (int id READ id)
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
public:
    explicit StandObj(QObject *parent = 0): QObject(parent), mStand(0) {}
    void setStand(const FMStand* stand) { mStand = stand; }
    double basalArea() const { return mStand->basalArea(); }
    double age() const {return mStand->age(); }
    double volume() const {return mStand->volume(); }
    int id() const { return mStand->id(); }
private:
    const FMStand *mStand;
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


} // namespace

#endif // FOMESCRIPT_H
