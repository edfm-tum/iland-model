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
    return QStringList() << QString("(accumulated) harvest: %1").arg(mRealizedHarvest)
                         << QString("MAI: %1").arg(mMAI)
                         << QString("HDZ: %1").arg(mHDZ)
                         << QString("average age: %1").arg(mMeanAge)
                         << QString("decadal plan: %1").arg(mAnnualHarvestTarget)
                         << QString("current plan: %1").arg(constScheduler()!=0?constScheduler()->harvestTarget():0.);

}

FMUnit::FMUnit(const Agent *agent)
{
    mAgent = agent;
    mScheduler = 0;
    mAnnualHarvestTarget = -1.;
    mRealizedHarvest = 0.;
    mMAI = 0.; mHDZ = 0.; mMeanAge = 0.;
    mTotalArea = 0.; mTotalPlanDeviation = 0.;
    mTotalVolume = 0.;
    mAnnualHarvest = 0.;

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
}

void FMUnit::managementPlanUpdate()
{
    const double period_length  = 10.;
    // calculate the planned harvest in the next planning period (i.e., 10yrs).
    // this is the sum of planned operations that are already in the scheduler.
    double planned = mScheduler->plannedHarvests(false);
    // the actual harvests of the last planning period
    double realized = mRealizedHarvest;
    // the plan of the last period
    double old_plan = mAnnualHarvestTarget * period_length;

    mRealizedHarvest = 0.; // reset
    mRealizedHarvestLastYear = 0.;


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
        mai += stand->meanAnnualIncrementTotal() * area; // m3/yr
        age += stand->absoluteAge() * area;
        volume += stand->volume() * area;
        // HDZ: "haubarer" average increment: timber that is ready for final harvest
        if (stand->readyForFinalHarvest())
            hdz += stand->volume() / stand->absoluteAge() * area;
        total_area += area;
        ++it;
    }
    mTotalArea = total_area;
    if (total_area==0.)
        return;

    mai /= total_area; // m3/ha*yr area weighted average of annual increment
    age /= total_area; // area weighted mean age
    hdz /= total_area; // =sum(Vol/age * share)

    mMAI = mai;
    mHDZ = hdz;
    mMeanAge = age;
    mTotalVolume = volume;

    double rotation_length = 100.; // TODO
    double h_tot = mai * 2.*age / rotation_length;  //
    double h_reg = hdz * 2.*age / rotation_length;
    double h_thi = h_tot - h_reg;

    qCDebug(abe) << "plan-update for unit" << id() << ": h-tot:" << h_tot << "h_reg:" << h_reg << "h_thi:" << h_thi << "of total volume:" << volume;
    if (!mAgent->useSustainableHarvest()) {
        // we do not calculate sustainable harvest levels.
        // do a pure bottom up calculation
        mAnnualHarvestTarget = planned / period_length;
        qCDebug(abe) << "unit" << id() << "new plan:" << mAnnualHarvestTarget;
    } else {
        // use the sustainable harvest level.
        mAnnualHarvestTarget = h_tot;
        if (old_plan>0.) {
            double delta = (realized-old_plan) / period_length;
            // if delta > 0: timber removal was too high -> plan less for the current period, and vice versa.
            mAnnualHarvestTarget -= delta;
        }
        mAnnualHarvestTarget = qMax(mAnnualHarvestTarget, 0.);
    }
    if (scheduler())
        scheduler()->setHarvestTarget(mAnnualHarvestTarget);
}

void FMUnit::updatePlanOfCurrentYear()
{
    if (!scheduler())
        return;

    if (mTotalArea==0.)
        throw IException("FMUnit:updatePlan: unit area = 0???");

    // compare the harvests of the last year to the plan:
    double harvests = mRealizedHarvest - mRealizedHarvestLastYear;
    mRealizedHarvestLastYear = mRealizedHarvest;
    mAnnualHarvest = harvests;

    // difference in m3/ha
    double delta = harvests/mTotalArea - mAnnualHarvestTarget;
    mTotalPlanDeviation += delta;

    // apply decay function for deviation
    mTotalPlanDeviation *= mAgent->type()->schedulerOptions().deviationDecayRate;

    // relative deviation: >0: too many harvests
    double rel_deviation = mAnnualHarvestTarget? mTotalPlanDeviation / mAnnualHarvestTarget : 0;

    // the current deviation is reduced to 0 in rebounce_yrs years.
    double rebounce_yrs = mAgent->type()->schedulerOptions().scheduleRebounceDuration;

    double new_harvest = mAnnualHarvestTarget * (1. - rel_deviation/rebounce_yrs);

    // limit to minimum/maximum parameter
    new_harvest = qMax(new_harvest, mAgent->type()->schedulerOptions().minScheduleHarvest);
    new_harvest = qMin(new_harvest, mAgent->type()->schedulerOptions().maxScheduleHarvest);
    scheduler()->setHarvestTarget(new_harvest);

//    const QMultiMap<FMUnit*, FMStand*> &stands = ForestManagementEngine::instance()->stands();
//    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stands.constFind(this);
//    while (it != stands.constEnd() && it.key()==this) {
//        FMStand *stand = it.value();
//        ++it;
//    }
}

} // namesapce