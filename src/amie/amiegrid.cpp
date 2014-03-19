#include "amie_global.h"
#include "global.h"
#include "amiegrid.h"
#include "fmstand.h"
#include "fmunit.h"
#include "modelcontroller.h"



double AMIELayers::value(const FMStandPtr &data, const int index) const
{
    if (data == 0 && index<2) return -1; // for classes
    if (data == 0) return 0;
    switch (index) {
    case 0:
        if(!mStandIndex.contains(data->id()))
            mStandIndex[data->id()] = mStandIndex.count();
        return mStandIndex[data->id()]; // "id"
    case 1:
        if(!mUnitIndex.contains(data->unit()->id()))
            mUnitIndex[data->unit()->id()] = mUnitIndex.count();
        return mUnitIndex[data->unit()->id()]; // unit
    case 2: return 0; // "agent"
    case 3: return data->volume(); // "volume"
    case 4: return data->basalArea(); // "basalArea"
    case 5: return data->age(); // "age"
    case 6: return data->sleepYears(); // "next evaluation"
    default: throw IException("AMIELayers:value(): Invalid index");
    }
}

const QVector<LayeredGridBase::LayerElement> AMIELayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("id"), QLatin1Literal("stand ID"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QLatin1Literal("unit"), QLatin1Literal("ID of the management unit"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QLatin1Literal("agent"), QLatin1Literal("managing agent"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QLatin1Literal("volume"), QLatin1Literal("stocking volume (m3/ha)"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QLatin1Literal("basalArea"), QLatin1Literal("stocking basal area (m2/ha)"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QLatin1Literal("age"), QLatin1Literal("stand age"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QLatin1Literal("next evaluation"), QLatin1Literal("years until the next evaluation"), GridViewHeat);
}

const QString AMIELayers::labelvalue(const int value, const int index) const
{
    switch(index) {
    case 0: // stand id
        return QString::number(mStandIndex.key(value));
    case 1: // unit
        return mUnitIndex.key(value);
    default: return QString::number(value);
    }
}

void AMIELayers::registerLayers()
{
    GlobalSettings::instance()->controller()->addLayers(this, "AMIE");
}

void AMIELayers::clearClasses()
{
    mAgentIndex.clear();
    mStandIndex.clear();
    mUnitIndex.clear();
}
