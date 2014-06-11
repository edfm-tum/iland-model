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
    double area() const { return mTotalArea; } ///< total area of the unit (ha)

    // actions

    /// update decadal management objectives for the planning unit.
    void managementPlanUpdate();

    /// update objectives of the current year.
    void updatePlanOfCurrentYear();

    /// record realized harvests on the unit (final harvests)
    void addRealizedHarvest(const double harvest_m3) { mRealizedHarvest+=harvest_m3; }

    void aggregate();
    QStringList info() const;

private:
    double annualFinalHarvest() const {return mRealizedHarvest-mRealizedHarvestLastYear; } ///< total m3 produced in final harvests in this year
    double annualThinningHarvest() const; ///< return the total m3 of thinning harvests (m3)
    QString mId;
    const Agent *mAgent;
    Scheduler *mScheduler;
    double mAnnualHarvestTarget; ///< planned annual harvest (final harvests) (m3)
    double mAnnualThinningTarget; ///< planned annual harvests (thinnings and tendings) (m3)
    double mRealizedHarvest; ///< sum of realized harvest in the current planning period (final harvests) (m3)
    double mRealizedHarvestLastYear; ///< the sum of harvests up to the last year (final harvests) (m3)
    double mAnnualHarvest; ///< suf of the harvest of the current year (final harvests)
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
