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
    SchedulerOptions(): useScheduler(false), useSustainableHarvest(1.), minScheduleHarvest(0), maxScheduleHarvest(0), maxHarvestOvershoot(0), harvestIntensity(1.), scheduleRebounceDuration(0), deviationDecayRate(0.), balanceWorkload(0.){ minPriorityFormula = 0; }
    ~SchedulerOptions();
    bool useScheduler; ///< true, if the agent is using the scheduler at all
    double useSustainableHarvest; ///< scaling factor (0..1), 1 if scheduler used by agent (exclusively), 0: bottom up, linearly scaled in between.
    double minScheduleHarvest; ///< minimum amount of m3/ha*yr that should be scheduled
    double maxScheduleHarvest; ///< the maximum number of m3/ha*yr that should be scheduled
    double maxHarvestOvershoot; ///< multiplier to define the maximum overshoot over the planned volume (e.g. 1.2 -> 20% max. overshoot)
    double harvestIntensity; ///< multiplier for the "sustainable" harvest level
    double scheduleRebounceDuration; ///< number of years for which deviations from the planned volume are split into
    double deviationDecayRate; ///< factor to reduce accumulated harvest deviation
    double balanceWorkload; ///< factor between 0..1; 1: use only the minPriority formula, 0: no balancing
    Expression *minPriorityFormula; ///< formula to determine the minimum required activity rating for a given amount of harvest objective achievment.

    void setup(QJSValue jsvalue);
    static QStringList mAllowedProperties;
};

/**
 * @brief The Scheduler class schedules the forest management activities
 * on a planning unit.
 *
 */
class Scheduler
{
public:
    Scheduler(FMUnit* unit) { mUnit = unit; mExtraHarvest=0.; mFinalCutTarget=0.; }
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
    double plannedHarvests(double &rFinal, double &rThinning);

    /// set the harvest target for the unit (m3/ha) for the current year.
    /// target_m3_ha: the
    void setHarvestTarget(double target_m3_ha, double thinning_target_m3_ha) { mFinalCutTarget = std::max(target_m3_ha,0.01);
                                                                               mThinningTarget = std::max(thinning_target_m3_ha,0.01); }
    double harvestTarget() const { return mFinalCutTarget; }

    /// get current score for stand 'id'
    /// return -1 if stand is invalid, 0..1 for probabilities, 1.1 for forced execution
    double scoreOf(const int stand_id) const;
    QStringList info(const int stand_id) const;

private:
    double calculateMinProbability(const double current_harvest);
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
        int scheduledYear; ///< planned execution year
        int forbiddenTo; ///< year until which the harvest operation is forbidden
        ActivityFlags *flags; ///< the details of the activity/stand context
    };
    struct ItemComparator
    {
        bool operator()( const SchedulerItem *lx, const SchedulerItem *rx ) const;
    };
    QList<SchedulerItem*> mItems; ///< the list of active tickets
    QMultiHash<int, SchedulerItem*> mSchedule;
    /// find scheduler item for 'stand_id' or return NULL.
    SchedulerItem* item(const int stand_id) const;
    FMUnit *mUnit;
    double mExtraHarvest; ///< extra harvests due to disturbances m3
    double mFinalCutTarget; ///< current harvest target for regeneration harvests (m3/ha)
    double mThinningTarget; ///< current harvest target for thinning/tending operations (m3/ha)

    static const int MAX_YEARS = 20;
    friend class UnitOut;
};



} // namespace
#endif // SCHEDULER_H
