#include "abe_global.h"
#include "scheduler.h"

#include "fmstand.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstp.h"
#include "forestmanagementengine.h"
#include "agent.h"
#include "agenttype.h"

#include "mapgrid.h"
#include "expression.h"

namespace ABE {


void Scheduler::addTicket(FMStand *stand, ActivityFlags *flags, double prob_schedule, double prob_execute)
{
    if (FMSTP::verbose())
        qDebug()<< "ticked added for stand" << stand->id();
    
    flags->setIsPending(true);
    SchedulerItem *item = new SchedulerItem();
    item->stand = stand;
    item->flags = flags;
    item->harvest = stand->scheduledHarvest();
    item->harvestPerHa = stand->scheduledHarvest() / stand->area();
    item->harvestType = flags->isFinalHarvest()? EndHarvest : Thinning;
    item->scheduleScore = prob_schedule;
    item->harvestScore = prob_execute;
    item->enterYear = ForestManagementEngine::instance()->currentYear();
    item->optimalYear = item->enterYear + flags->activity()->optimalSchedule(stand->stp()->rotationLength())- stand->absoluteAge();
    item->forbiddenTo = 0;
    item->calculate(); // set score
    mItems.push_back(item);
}


void Scheduler::run()
{
    // update the plan if necessary...
    if (FMSTP::verbose() && mItems.size()>0)
        qCDebug(abe) << "running scheduler for unit" << mUnit->id() << ". # of active items:" << mItems.size();

    double harvest_in_queue = 0.;
    double total_final_harvested = mExtraHarvest;
    double total_thinning_harvested = 0.;
    mExtraHarvest = 0.;
    if (FMSTP::verbose() && total_final_harvested>0.)
        qCDebug(abe) << "Got extra harvest (e.g. salvages), m3=" << total_final_harvested;

    // update the schedule probabilities....
    QList<SchedulerItem*>::iterator it = mItems.begin();
    while (it!=mItems.end()) {
        SchedulerItem *item = *it;
        harvest_in_queue += item->stand->scheduledHarvest();
        double p_sched = item->flags->activity()->scheduleProbability(item->stand);
        item->scheduleScore = p_sched;
        item->calculate();
        if (item->stand->trace())
            qCDebug(abe) << item->stand->context() << "scheduler scores (harvest schedule total): " << item->harvestScore << item->scheduleScore << item->score;

        // drop item if no schedule to happen any more
        if (item->score == 0.) {
            if (item->stand->trace())
                qCDebug(abe) << item->stand->context() << "dropped activity" << item->flags->activity()->name() << "from scheduler.";
            item->stand->afterExecution(true); // execution canceled
            it = mItems.erase(it);
            delete item;
        } else {
            ++it;
        }
    }

    // sort the probabilities, highest probs go first....
    qSort(mItems);
    if (FMSTP::verbose())
        dump();

    int no_executed = 0;
    double harvest_scheduled = 0.;
    int current_year = ForestManagementEngine::instance()->currentYear();
    // now execute the activities with the highest ranking...

    it = mItems.begin();
    while (it!=mItems.end()) {
        SchedulerItem *item = *it;
        // ignore stands that are currently banned
        if (item->forbiddenTo > current_year) {
            ++it;
            continue;
        }

        bool remove = false;
        bool final_harvest = item->flags->isFinalHarvest();
        //
        double rel_harvest;
        if (final_harvest)
            rel_harvest = total_final_harvested / mUnit->area() / mFinalCutTarget;
        else
            rel_harvest = total_thinning_harvested/mUnit->area() / mThinningTarget;
        double min_exec_probability = calculateMinProbability( rel_harvest );


        if (item->score >= min_exec_probability) {

            // execute activity:
            if (item->stand->trace())
                qCDebug(abe) << item->stand->context() << "execute activity" << item->flags->activity()->name() << "score" << item->score << "planned harvest:" << item->stand->scheduledHarvest();
            harvest_scheduled += item->stand->scheduledHarvest();

            bool executed = item->flags->activity()->execute(item->stand);
            if (final_harvest)
                total_final_harvested += item->stand->totalHarvest();
            else
                total_thinning_harvested += item->stand->totalHarvest();

            item->flags->setIsPending(false);
            if (!item->flags->activity()->isRepeatingActivity()) {
                item->flags->setActive(false);
                item->stand->afterExecution(!executed); // check what comes next for the stand
            }
            no_executed++;

            // flag neighbors of the stand, if a clearcut happened
            // this is to avoid large unforested areas
            if (executed && final_harvest) {
                if (FMSTP::verbose()) qCDebug(abe) << item->stand->context() << "ran final harvest -> flag neighbors";
                // simple rule: do not allow harvests for neighboring stands for 5 years
                item->forbiddenTo = current_year + 5;
                QList<int> neighbors = ForestManagementEngine::instance()->standGrid()->neighborsOf(item->stand->id());
                for (QList<SchedulerItem*>::iterator nit = mItems.begin(); nit!=mItems.end(); ++nit)
                    if (neighbors.contains((*nit)->stand->id()))
                        (*nit)->forbiddenTo = current_year + 5;

            }

            remove = true;

        }
        if (remove) {
            // removing item from scheduler
            if (item->stand->trace())
                qCDebug(abe) << item->stand->context() << "removing activity" << item->flags->activity()->name() << "from scheduler.";
            it = mItems.erase(it);
            delete item;

        } else {
            ++it;
        }
    }
    if (FMSTP::verbose() && no_executed>0)
        qCDebug(abe) << "scheduler finished for" << mUnit->id() << ". # of items executed (n/volume):" << no_executed << "(" << harvest_scheduled << "m3), total:" << mItems.size() << "(" << harvest_in_queue << "m3)";

}

bool Scheduler::forceHarvest(const FMStand *stand, const int max_years)
{
    // check if we have the stand in the list:
     for (QList<SchedulerItem*>::const_iterator nit = mItems.constBegin(); nit!=mItems.constEnd(); ++nit) {
         const SchedulerItem *item = *nit;
         if (item->stand == stand)
             if (abs(item->optimalYear -  GlobalSettings::instance()->currentYear()) < max_years ) {
                 item->flags->setExecuteImmediate(true);
                 return true;
             }
     }
     return false;
}

void Scheduler::addExtraHarvest(const FMStand *stand, const double volume, Scheduler::HarvestType type)
{
    Q_UNUSED(stand); Q_UNUSED(type); // at least for now
    mExtraHarvest += volume;
}

double Scheduler::plannedHarvests(double &rFinal, double &rThinning)
{
    rFinal = 0.; rThinning = 0.;
    int current_year = ForestManagementEngine::instance()->currentYear();
    for (QList<SchedulerItem*>::const_iterator nit = mItems.constBegin(); nit!=mItems.constEnd(); ++nit)
        if (rFinal || (*nit)->optimalYear < current_year + 10)
            if ((*nit)->flags->isFinalHarvest()) {
                rFinal += (*nit)->harvest; // scheduled harvest in m3
            } else {
                rThinning += (*nit)->harvest;
            }

    return rFinal + rThinning;

}

double Scheduler::scoreOf(const int stand_id) const
{
    // lookup stand in scheduler list
    SchedulerItem *item = 0;
    for (QList<SchedulerItem*>::const_iterator nit = mItems.constBegin(); nit!=mItems.constEnd(); ++nit)
        if ((*nit)->stand->id() == stand_id) {
            item = *nit;
            break;
        }
    if (!item)
        return -1;

    return item->score;
}

QStringList Scheduler::info(const int stand_id) const
{
    SchedulerItem *si = item(stand_id);
    if (!si)
        return QStringList();
    QStringList lines = QStringList();
    lines << "-";
    lines << QString("type: %1").arg(si->harvestType==Thinning?QStringLiteral("Thinning"):QStringLiteral("End harvest"));
    lines << QString("schedule score: %1").arg(si->scheduleScore);
    lines << QString("total score: %1").arg(si->score);
    lines << QString("scheduled vol/ha: %1").arg(si->harvestPerHa);
    lines << QString("postponed to year: %1").arg(si->forbiddenTo);
    lines << QString("in scheduler since: %1").arg(si->enterYear);
    lines << "/-";
    return lines;
}

/// calculate the result of the response function that indicates, given the accumulated harvest of the current year,
/// the minimum priority ranking of stands that should be executed.
/// The idea: if the harvest level is low, the scheduler tends to run also harvests with low priority; if the
/// accumulated harvest approaches the target, the scheduler is increasingly picky.
double Scheduler::calculateMinProbability(const double current_harvest)
{
    if (current_harvest > mUnit->agent()->type()->schedulerOptions().maxHarvestOvershoot)
        return 999.; // never reached

    // use the provided equation
    double value =  mUnit->agent()->type()->schedulerOptions().minPriorityFormula->calculate(current_harvest);
    // use the balanceWorkload property
    double balance = mUnit->agent()->type()->schedulerOptions().balanceWorkload;
    value = balance * value + (1. - balance)*value;
    return std::min( std::max( value, 0.), 1.);
}

void Scheduler::dump()
{
    if(mItems.isEmpty())
        return;
    qCDebug(abe)<< "***** Scheduler items **** Unit:" << mUnit->id();
    qCDebug(abe)<< "stand.id, score, opt.year, act.name, planned.harvest";
    QList<SchedulerItem*>::iterator it = mItems.begin();
    while (it!=mItems.end()) {
        SchedulerItem *item = *it;
        qCDebug(abe) << QString("%1, %2, %3, %4, %5").arg(item->stand->id()).arg(item->score).arg(item->optimalYear)
                                                    .arg(item->flags->activity()->name())
                                                    .arg(item->harvest);
        ++it;
    }
}

Scheduler::SchedulerItem *Scheduler::item(const int stand_id) const
{
    for (QList<SchedulerItem*>::const_iterator nit = mItems.constBegin(); nit!=mItems.constEnd(); ++nit)
        if ((*nit)->stand->id() == stand_id) {
            return *nit;
        }
    return 0;
}

bool Scheduler::SchedulerItem::operator<(const Scheduler::SchedulerItem &item)
{
    // sort *descending*, i.e. after sorting, the item with the highest score is in front.
//    if (this->score == item.score)
//        return this->enterYear < item.enterYear; // higher prob. for items that entered earlier TODO: change to due/overdue
    return this->score > item.score;
}

void Scheduler::SchedulerItem::calculate()
{
    if (flags->isExecuteImmediate())
        score = 1.1; // above 1
    else
        score = scheduleScore * harvestScore;
}


// **************************************************************************************
SchedulerOptions::~SchedulerOptions()
{
    if (minPriorityFormula)
        delete minPriorityFormula;
}

void SchedulerOptions::setup(QJSValue jsvalue)
{
    useScheduler = false;
    if (!jsvalue.isObject())
        return;
    minScheduleHarvest = FMSTP::valueFromJs(jsvalue, "minScheduleHarvest","0").toNumber();
    maxScheduleHarvest = FMSTP::valueFromJs(jsvalue, "maxScheduleHarvest","10000").toNumber();
    maxHarvestOvershoot = FMSTP::valueFromJs(jsvalue, "maxHarvestOvershoot","2").toNumber();
    useSustainableHarvest = FMSTP::valueFromJs(jsvalue, "useSustainableHarvest", "1").toNumber();
    if (useSustainableHarvest<0. || useSustainableHarvest>1.)
        throw IException("Setup of scheduler-options: invalid value for 'useSustainableHarvest' (0..1 allowed).");

    scheduleRebounceDuration = FMSTP::valueFromJs(jsvalue, "scheduleRebounceDuration", "5").toNumber();
    if (scheduleRebounceDuration==0.)
        throw IException("Setup of scheduler-options: '0' is not a valid value for 'scheduleRebounceDuration'!");
    // calculate the "tau" of a exponential decay function based on the provided half-time
    scheduleRebounceDuration = scheduleRebounceDuration / log(2.);
    deviationDecayRate = FMSTP::valueFromJs(jsvalue, "deviationDecayRate","0").toNumber();
    if (deviationDecayRate==1.)
        throw IException("Setup of scheduler-options: '0' is not a valid value for 'deviationDecayRate'!");
    deviationDecayRate = 1. - deviationDecayRate; // if eg value is 0.05 -> multiplier 0.95
    if (!minPriorityFormula)
        minPriorityFormula = new Expression();
    minPriorityFormula->setExpression(FMSTP::valueFromJs(jsvalue, "minPriorityFormula","x^4").toString());
    balanceWorkload = FMSTP::valueFromJs(jsvalue, "balanceWorkload", "0.5").toNumber();
    useScheduler = true;

}


} // namespace
