#include "amie_global.h"
#include "global.h"
#include "amiegrid.h"
#include "fmstand.h"
#include "fmunit.h"
#include "modelcontroller.h"
#include "scheduler.h"



AMIELayers::~AMIELayers()
{
    GlobalSettings::instance()->controller()->removeLayers(this);
}

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
    case 7: return data->lastUpdate(); // last update
    case 8: return data->unit()->constScheduler()?data->unit()->constScheduler()->scoreOf(data->id()) : -1.; // scheduler score
    default: throw IException("ABELayers:value(): Invalid index");
    }
}

const QVector<LayeredGridBase::LayerElement> AMIELayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QStringLiteral("id"), QStringLiteral("stand ID"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QStringLiteral("unit"), QStringLiteral("ID of the management unit"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QStringLiteral("agent"), QStringLiteral("managing agent"), GridViewBrewerDiv)
            << LayeredGridBase::LayerElement(QStringLiteral("volume"), QStringLiteral("stocking volume (m3/ha)"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QStringLiteral("basalArea"), QStringLiteral("stocking basal area (m2/ha)"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QStringLiteral("age"), QStringLiteral("stand age"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QStringLiteral("next evaluation"), QStringLiteral("years until the next evaluation"), GridViewHeat)
            << LayeredGridBase::LayerElement(QStringLiteral("last update"), QStringLiteral("year of the last update of the forest state."), GridViewRainbowReverse)
            << LayeredGridBase::LayerElement(QStringLiteral("scheduler score"), QStringLiteral("score of a stand in the scheduler (higher scores: higher prob. to be executed)."), GridViewRainbow);
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
