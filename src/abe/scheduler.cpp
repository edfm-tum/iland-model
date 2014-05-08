#include "amie_global.h"
#include "scheduler.h"

#include "fmstand.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstp.h"
#include "forestmanagementengine.h"

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
    item->harvestType = flags->isFinalHarvest()? SchedulerItem::EndHarvest : SchedulerItem::Thinning;
    item->scheduleScore = prob_schedule;
    item->harvestScore = prob_execute;
    item->enterYear = ForestManagementEngine::instance()->currentYear();
    item->forbiddenTo = 0;
    item->calculate(); // set score
    mItems.push_back(item);
}


void Scheduler::run()
{
    // update the plan if necessary...
    if (FMSTP::verbose() && mItems.size()>0)
        qCDebug(amie) << "running scheduler for unit" << mUnit->id() << ". # of active items:" << mItems.size();

    double harvest_in_queue = 0.;
    double total_harvested = mExtraHarvest;
    mExtraHarvest = 0.;

    // update the schedule probabilities....
    QList<SchedulerItem*>::iterator it = mItems.begin();
    while (it!=mItems.end()) {
        SchedulerItem *item = *it;
        harvest_in_queue += item->stand->scheduledHarvest();
        double p_sched = item->flags->activity()->scheduleProbability(item->stand);
        item->scheduleScore = p_sched;
        item->calculate();
        if (item->stand->trace())
            qCDebug(amie) << item->stand->context() << "scheduler scores (harvest schedule total): " << item->harvestScore << item->scheduleScore << item->score;

        // drop item if no schedule to happen any more
        if (item->score == 0.) {
            if (item->stand->trace())
                qCDebug(amie) << item->stand->context() << "dropped activity" << item->flags->activity()->name() << "from scheduler.";
            item->stand->afterExecution(true); // execution canceled
            it = mItems.erase(it);
            delete item;
        } else {
            ++it;
        }
    }

    // sort the probabilities, highest probs go first....
    qSort(mItems);

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
        //
        double min_exec_probability = calculateMinProbability(total_harvested);


        if (item->score >= min_exec_probability) {

            // execute activity:
            if (item->stand->trace())
                qCDebug(amie) << item->stand->context() << "execute activity" << item->flags->activity()->name() << "score" << item->score << "planned harvest:" << item->stand->scheduledHarvest();
            harvest_scheduled += item->stand->scheduledHarvest();

            bool executed = item->flags->activity()->execute(item->stand);
            total_harvested += item->stand->totalHarvest();

            item->flags->setIsPending(false);
            if (!item->flags->activity()->isRepeatingActivity()) {
                item->flags->setActive(false);
                item->stand->afterExecution(!executed); // check what comes next for the stand
            }
            no_executed++;

            // flag neighbors of the stand, if a clearcut happened
            // this is to avoid large unforested areas
            if (executed && item->flags->isFinalHarvest()) {
                if (FMSTP::verbose()) qCDebug(amie) << item->stand->context() << "ran final harvest -> flag neighbors";
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
                qCDebug(amie) << item->stand->context() << "removing activity" << item->flags->activity()->name() << "from scheduler.";
            it = mItems.erase(it);
            delete item;

        } else {
            ++it;
        }
    }
    if (FMSTP::verbose() && no_executed>0)
        qCDebug(amie) << "scheduler finished for" << mUnit->id() << ". # of items executed (n/volume):" << no_executed << "(" << harvest_scheduled << "m3), total:" << mItems.size() << "(" << harvest_in_queue << "m3)";

}

bool Scheduler::forceHarvest(const FMStand *stand, const int max_years)
{
    // check if we have the stand in the list:
     for (QList<SchedulerItem*>::const_iterator nit = mItems.constBegin(); nit!=mItems.constEnd(); ++nit) {
         if ((*nit)->stand == stand)
             if ((*nit)->optimalYear - max_years > GlobalSettings::instance()->currentYear()) {
                 (*nit)->flags->setExecuteImmediate(true);
                 return true;
             }
     }
     return false;
}

void Scheduler::addExtraHarvest(const FMStand *stand, const double volume, Scheduler::HarvestType type)
{
    mExtraHarvest += volume;
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
    lines << QString("type: %1").arg(si->harvestType==SchedulerItem::Thinning?QStringLiteral("Thinning"):QStringLiteral("End harvest"));
    lines << QString("schedule score: %1").arg(si->scheduleScore);
    lines << QString("total score: %1").arg(si->score);
    lines << QString("scheduled vol/ha: %1").arg(si->harvestPerHa);
    lines << QString("postponed to year: %1").arg(si->forbiddenTo);
    lines << QString("in scheduler since: %1").arg(si->enterYear);
    lines << "/-";
    return lines;
}

double Scheduler::calculateMinProbability(double current_harvest)
{
    return 0.5;
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
    // sort *descending*, i.e. after sorting the item with the highest score is in front.
    if (this->score == item.score)
        return this->enterYear < item.enterYear; // higher prob. for items that entered earlier TODO: change to due/overdue
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
    if (minRating)
        delete minRating;
}

void SchedulerOptions::setup(QJSValue jsvalue)
{
    useScheduler = false;
    if (!jsvalue.isObject())
        return;
    minScheduleHarvest = FMSTP::valueFromJs(jsvalue, "minScheduleHarvest","0").toNumber();
    maxScheduleHarvest = FMSTP::valueFromJs(jsvalue, "maxScheduleHarvest","10000").toNumber();
    maxHarvestOvershoot = FMSTP::valueFromJs(jsvalue, "maxHarvestOvershoot","2").toNumber();
    scheduleRebounceDuration = FMSTP::valueFromJs(jsvalue, "scheduleRebounceDuration", "5").toNumber();
    if (!minRating)
        minRating = new Expression();
    minRating->setExpression(FMSTP::valueFromJs(jsvalue, "minRatingFormula","1").toString());
    useScheduler = true;

}


} // namespace
