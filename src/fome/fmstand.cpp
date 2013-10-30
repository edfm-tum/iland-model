#include "fome_global.h"
#include "fmstand.h"

#include "fmunit.h"

FMStand::FMStand(FMUnit *unit, const int id)
{
    mUnit = unit;
    mId = id;
    mPhase = Activity::Invalid;
    // testing:
    mPhase = Activity::Tending;
    mStandType = 1; // just testing...
    mVolume = 0.;
    mAge = 0.;
    mBasalArea = 0.;
}

double FMStand::basalArea(const QString &species_id) const
{
    // get somehow the value from the stand directly or the stand meta data....
    return 0;
}

// storage for properties (static)
QHash<FMStand*, QHash<QString, QJSValue> > FMStand::mStandPropertyStorage;


void FMStand::setProperty(const QString &name, QJSValue value)
{
    // save a property value for the current stand
    mStandPropertyStorage[this][name] = value;
}

QJSValue FMStand::property(const QString &name)
{
    // check if values are already stored for the current stand
    if (!mStandPropertyStorage.contains(this))
        return QJSValue();
    // check if something is stored for the property name (return a undefined value if not)
    if (!mStandPropertyStorage[this].contains(name))
        return QJSValue();
    return mStandPropertyStorage[this][name];
}
