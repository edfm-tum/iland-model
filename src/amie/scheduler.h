#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <QList>

#include "activity.h"

namespace AMIE {
class FMStand; // forward
class FMUnit; // forward

/**
 * @brief The Scheduler class schedules the forest management activities
 * on a planning unit.
 *
 */
class Scheduler
{
public:
    Scheduler(FMUnit* unit) { mUnit = unit; }

    /// add an planned activity for a given stand.
    /// @param stand the stand to add
    /// @param flags the execution flags (activty x stand)
    /// @param prob_schedule the probability from the activity-scheduling algorithm at the time of adding the ticket
    /// @param prob_execute the probability for executing the activity (based on the constraints of the activity)
    void addTicket(FMStand *stand, ActivityFlags *flags, double prob_schedule, double prob_execute);

    /// executes the scheduler for the planning unit.
    /// scheduled operations are executed.
    void run();

    /// get current score for stand 'id'
    /// return -1 if stand is invalid, 0..1 for probabilities, 1.1 for forced execution
    double scoreOf(const int stand_id) const;

private:
    class SchedulerItem {
    public:
        SchedulerItem(): stand(0), score(0.) {}
        bool operator<(const SchedulerItem &item);
        void calculate(); ///< calculate the final score
        FMStand *stand; ///< the stand to be harvested
        double harvest; ///< the scheduled harvest in m3
        double harvestPerHa; ///< harvest per ha
        double scheduleScore; ///< the probability based on schedule timing
        double harvestScore; ///< the probability of the activity
        double score; ///< the total score of this ticked to be executed this year
        enum HarvestType { Thinning, EndHarvest} harvestType; ///< type of harvest
        int  enterYear; ///< the year the ticket was created
        int forbiddenTo; ///< year until which any harvest operation is forbidden
        ActivityFlags *flags; ///< the details of the activity/stand context
    };
    QList<SchedulerItem*> mItems; ///< the list of active tickets
    FMUnit *mUnit;
};



} // namespace
#endif // SCHEDULER_H
