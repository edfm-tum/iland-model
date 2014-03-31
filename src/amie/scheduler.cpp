#include "amie_global.h"
#include "scheduler.h"

#include "fmstand.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstp.h"
#include "forestmanagementengine.h"

#include "mapgrid.h"

namespace AMIE {


void Scheduler::addTicket(FMStand *stand, ActivityFlags *flags, double prob_schedule, double prob_execute)
{
    // for the time being, force execution
    if (FMSTP::verbose())
        qDebug()<< "ticked added for stand" << stand->id();

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

    // update the schedule probabilities....
    for (QList<SchedulerItem*>::iterator it = mItems.begin(); it!=mItems.end(); ++it) {
        SchedulerItem *item = *it;
        double p_sched = item->flags->activity()->scheduleProbability(item->stand);
        item->scheduleScore = p_sched;
        item->calculate();
        // drop item if no schedule to happen any more
        if (item->score == 0.) {
            if (item->stand->trace())
                qCDebug(abe) << item->stand->context() << "dropped activity" << item->flags->activity()->name() << "from scheduler.";
            item->stand->afterExecution(true); // execution canceled
            mItems.erase(it);
            delete item;
        }
    }

    // sort the probabilities, highest probs go first....
    qSort(mItems);

    int current_year = ForestManagementEngine::instance()->currentYear();
    // now execute the activities with the highest ranking...
    for (QList<SchedulerItem*>::iterator it = mItems.begin(); it!=mItems.end(); ++it) {
        // for the time being: execute everything >0.5 ... or if time > 10 yrs
        SchedulerItem *item = *it;
        // ignore stands that are currently banned
        if (item->forbiddenTo > current_year)
            continue;

        if (item->score > 0.5 || item->enterYear < current_year-10) {
            // execute activity:
            bool executed = item->flags->activity()->execute(item->stand);
            item->flags->setIsPending(false);
            item->flags->setActive(false); // done; TODO: check for repeating activities
            item->stand->afterExecution(!executed); // check what comes next for the stand
            // flag neighbors of the stand, if a clearcut happened
            // this is to avoid large unforested areas
            if (executed && item->flags->isFinalHarvest()) {
                // simple rule: do not allow harvests for neighboring stands for 5 years
                item->forbiddenTo = current_year + 5;
                QList<int> neighbors = ForestManagementEngine::instance()->standGrid()->neighborsOf(item->stand->id());
                for (QList<SchedulerItem*>::iterator nit = mItems.begin(); nit!=mItems.end(); ++nit)
                    if (neighbors.contains((*nit)->stand->id()))
                        (*nit)->forbiddenTo = current_year + 5;

            }


        }
    }
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

bool Scheduler::SchedulerItem::operator<(const Scheduler::SchedulerItem &item)
{
    return this->score < item.score;
}

void Scheduler::SchedulerItem::calculate()
{
    if (flags->isExecuteImmediate())
        score = 1.1; // above 1
    else
        score = scheduleScore * harvestScore;
}


} // namespace
