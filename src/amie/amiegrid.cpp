#include "amie_global.h"
#include "global.h"
#include "amiegrid.h"
#include "fmstand.h"
#include "fmunit.h"
#include "modelcontroller.h"



double AMIELayers::value(const FMStandPtr &data, const int index) const
{
    if (!data) return 0;
    switch (index) {
    case 0: return data->id(); // "id"
    case 1: return data->unit()->index(); // "unit"
    case 2: return 0; // "agent"
    case 3: return data->volume(); // "volume"
    case 4: return data->basalArea(); // "basalArea"
    case 5: return data->age(); // "age"
    case 6: return data->sleepYears(); // "next evaluation"
    default: throw IException("AMIELayers:value(): Invalid index");
    }
}

const QStringList AMIELayers::names() const
{
    return QStringList() << "id" << "unit" << "agent" << "volume" << "basalArea" << "age" << "next evaluation";
}

void AMIELayers::registerLayers()
{
    GlobalSettings::instance()->controller()->addLayers(this, "AMIE");
}
