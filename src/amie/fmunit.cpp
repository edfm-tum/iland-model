#include "amie_global.h"
#include "global.h"

#include "fmunit.h"

#include "forestmanagementengine.h"
#include "fmstand.h"
#include "scheduler.h"
#include "agent.h"
namespace AMIE {

void FMUnit::aggregate()
{
    // loop over all stands
    // collect some data....
    double age=0.;
    double volume = 0.;
    double harvest = 0.;
    double totalarea = 0.;
    const QMultiMap<FMUnit*, FMStand*> &stands = ForestManagementEngine::instance()->stands();
    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stands.constFind(this);
    while (it != stands.constEnd() && it.key()==this) {
        const FMStand *s = it.value();
        age += s->age() * s->area();
        volume += s->volume() * s->area();

        totalarea += s->area();
        ++it;
    }
    if (totalarea>0.) {
        age /= totalarea;
        volume /= totalarea;
        harvest /= totalarea;
    }
    qCDebug(abe) << "unit" << id() << "volume (m3/ha)" << volume << "age" << age << "planned harvest: todo";

}

QStringList FMUnit::info() const
{
    return QStringList() << "unit: todo";
}

FMUnit::FMUnit(const Agent *agent)
{
    mAgent = agent;
    // explicit scheduler only for stands/units that include more than one stand

    mScheduler = new Scheduler(this);
}

FMUnit::~FMUnit()
{
    if (mScheduler)
        delete mScheduler;
}

void FMUnit::setId(const QString &id)
{
    mId = id;
    mIndex = mId.toInt();
}

} // namesapce
