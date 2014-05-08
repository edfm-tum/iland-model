#include "abe_global.h"
#include "global.h"

#include "fmunit.h"

#include "forestmanagementengine.h"
#include "fmstand.h"
#include "scheduler.h"
#include "agent.h"
#include "agenttype.h"

namespace ABE {

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
    mScheduler = 0;

    if (agent->type()->schedulerOptions().useScheduler)
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

void FMUnit::managementPlanUpdate()
{
    // preparations:
    // MAI-calculation for all stands:
    double total_area = 0.;
    double age = 0.;
    double mai = 0.;
    double hdz = 0.;
    double volume = 0.;
    const QMultiMap<FMUnit*, FMStand*> &stands = ForestManagementEngine::instance()->stands();
    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stands.constFind(this);
    while (it != stands.constEnd() && it.key()==this) {
        FMStand *stand = it.value();
        stand->reload();
        stand->calculateMAI();
        // calculate sustainable total harvest (following Breymann)
        double area = stand->area();
        mai += stand->meanAnnualIncrement() * area; // m3/yr
        age += stand->age() * area;
        volume += stand->volume() * area;
        if (stand->age()>0.)
            hdz += stand->volume() / stand->age() * area;
        total_area += area;
        ++it;
    }
    if (total_area==0.)
        return;

    mai /= total_area; // m3/ha*yr area weighted average of annual increment
    age /= total_area; // area weighted mean age
    hdz /= total_area; // =sum(Vol/age * share)

    double rotation_length = 100.;
    double h_tot = mai * 2.*age / rotation_length;  //
    double h_reg = hdz * 2.*age / rotation_length;
    double h_thi = h_tot - h_reg;

    qCDebug(abe) << "plan-update for unit" << id() << ": h-tot:" << h_tot << "h_reg:" << h_reg << "h_thi:" << h_thi << "of total volume:" << volume;
}

void FMUnit::updatePlanOfCurrentYear()
{
    const QMultiMap<FMUnit*, FMStand*> &stands = ForestManagementEngine::instance()->stands();
    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stands.constFind(this);
    while (it != stands.constEnd() && it.key()==this) {
        FMStand *stand = it.value();
        ++it;
    }
}

} // namesapce
