#ifndef FMUNIT_H
#define FMUNIT_H
namespace ABE {


class Agent; // forward
class Scheduler;
/** The FMUnit represents a management unit, i.e. a collection of stands.
 *  */
class FMUnit
{
public:
    FMUnit(const Agent *agent);
    ~FMUnit();
    void setId(const QString &id);
    const QString &id() const {return mId; }
    Scheduler *scheduler() {return mScheduler; }
    const Scheduler *constScheduler() const { return mScheduler; }
    const Agent* agent() const { return mAgent; }
    double area() const { return mTotalArea; }

    // actions

    /// update decadal management objectives for the planning unit.
    void managementPlanUpdate();

    /// update objectives of the current year.
    void updatePlanOfCurrentYear();

    /// record realized harvests on the unit
    void addRealizedHarvest(const double harvest_m3) { mRealizedHarvest+=harvest_m3; }

    void aggregate();
    QStringList info() const;

private:
    double annualHarvest() const {return mRealizedHarvest-mRealizedHarvestLastYear; }
    QString mId;
    const Agent *mAgent;
    Scheduler *mScheduler;
    double mAnnualHarvestTarget; ///< planned annual harvest (m3)
    double mRealizedHarvest; ///< sum of realized harvest in the current planning period (m3)
    double mRealizedHarvestLastYear; ///< the sum of harvests up to the last year (m3)
    double mAnnualHarvest; ///< suf of the harvest of the current year
    double mMAI; ///< mean annual increment (m3/ha)
    double mHDZ; ///< mean "haubarer" annual increment (m3/ha)
    double mMeanAge; ///< mean age of the planning unit
    double mTotalArea; ///< total area of the unit (ha)
    double mTotalVolume; ///< total standing volume
    double mTotalPlanDeviation; ///< cumulative deviation from the planned harvest

    friend class UnitOut;
};

} // namespace
#endif // FOMEUNITS_H
