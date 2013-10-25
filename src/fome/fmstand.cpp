#include "fome_global.h"
#include "fmstand.h"

#include "fmunit.h"

FMStand::FMStand(FMUnit *unit)
{
    mUnit = unit;
    mId = 0;
    mPhase = Activity::Invalid;
    // testing:
    mPhase = Activity::Tending;
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
