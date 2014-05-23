#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <QList>

#include "activity.h"
class Expression;

namespace ABE {
class FMStand; // forward
class FMUnit; // forward

/** @brief SchedulerOptions store agent-specific options.
 * */
struct SchedulerOptions {
    SchedulerOptions(): useScheduler(false), minScheduleHarvest(0), maxScheduleHarvest(0), maxHarvestOvershoot(0), scheduleRebounceDuration(0), deviationDecayRate(0.) { minRating = 0; }
    ~SchedulerOptions();
    bool useScheduler; ///< true, if scheduler used by agent
    double minScheduleHarvest; ///< minimum amount of m3/ha*yr that should be scheduled
    double maxScheduleHarvest; ///< the maximum number of m3/ha*yr that should be scheduled
    double maxHarvestOvershoot; ///< multiplier to define the maximum overshoot over the planned volume (e.g. 1.2 -> 20% max. overshoot)
    double scheduleRebounceDuration; ///< number of years for which deviations from the planned volume are split into
    double deviationDecayRate; ///< factor to reduce accumulated harvest deviation
    Expression *minRating; ///< formula to determine the minimum required activity rating for a given amount of harvest objective achievment.
    void setup(QJSValue jsvalue);
};

/**
 * @brief The Scheduler class schedules the forest management activities
 * on a planning unit.
 *
 */
class Scheduler
{
public:
    Scheduler(FMUnit* unit) { mUnit = unit; mExtraHarvest=0.; mHarvestTarget=0.; }
    enum HarvestType { Thinning, EndHarvest, Salvage};

    /// add an planned activity for a given stand.
    /// @param stand the stand to add
    /// @param flags the execution flags (activty x stand)
    /// @param prob_schedule the probability from the activity-scheduling algorithm at the time of adding the ticket
    /// @param prob_execute the probability for executing the activity (based on the constraints of the activity)
    void addTicket(FMStand *stand, ActivityFlags *flags, double prob_schedule, double prob_execute);

    /// executes the scheduler for the planning unit.
    /// scheduled operations are executed.
    void run();

    /// prepone a stand if in queue for the given stand.
    /// return true if a activity is preponed.
    bool forceHarvest(const FMStand *stand, const int max_years);

    /// tell the scheduler about extra harvests (that should be considered in the scheduling)
    /// volume: total volume (m3)
    void addExtraHarvest(const FMStand *stand, const double volume, HarvestType type);

    /// return the total amount of planned harvests in the next planning period (10yrs) (total=false)
    /// if 'total' is true all scheduled harvests are counted
    double plannedHarvests(bool total);

    /// set the harvest target for the unit (m3/ha) for the current year.
    void setHarvestTarget(double target_m3_ha) { mHarvestTarget = target_m3_ha; }
    double harvestTarget() const { return mHarvestTarget; }

    /// get current score for stand 'id'
    /// return -1 if stand is invalid, 0..1 for probabilities, 1.1 for forced execution
    double scoreOf(const int stand_id) const;
    QStringList info(const int stand_id) const;

private:
    double calculateMinProbability(double current_harvest);
    void updateCurrentPlan();
    void dump();
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
        HarvestType harvestType; ///< type of harvest
        int  enterYear; ///< the year the ticket was created
        int  optimalYear; ///< the (first) year where execution is considered as optimal
        int forbiddenTo; ///< year until which the harvest operation is forbidden
        ActivityFlags *flags; ///< the details of the activity/stand context
    };
    QList<SchedulerItem*> mItems; ///< the list of active tickets
    /// find scheduler item for 'stand_id' or return NULL.
    SchedulerItem* item(const int stand_id) const;
    FMUnit *mUnit;
    double mExtraHarvest;
    double mHarvestTarget; ///< current harvest target (m3/ha)

    friend class UnitOut;
};



} // namespace
#endif // SCHEDULER_H
