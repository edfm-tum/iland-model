#ifndef FMSTAND_H
#define FMSTAND_H

#include <QHash>
#include <QJSValue>

#include "activity.h"

class Species; // forward (iLand species)
class Tree; // forward (iLand tree)
enum TreeRemovalType; // forward

namespace ABE {

class FOMEWrapper; // forward
class FMUnit; // forward

struct SSpeciesStand
{
    SSpeciesStand(): species(0), basalArea(0.), relBasalArea(0.) {}
    const Species *species; ///< the ID of the species (ie a pointer)
    double basalArea; ///< basal area m2
    double relBasalArea; ///< fraction [0..1] fraction of species based on basal area.
};

/** FMStand encapsulates a forest stand for the forest management engine.
 *  The spatial coverage is defined by a "stand grid".
 * */

class FMStand
{
public:
    /// c'tor: link stand to a forest management unit
    FMStand(FMUnit *unit, const int id);
    /// set the stand to be managed by a given 'stp'
    void initialize(FMSTP *stp);
    /// sets the STP but nothing else (after disturbance related clearance)
    void reset(FMSTP *stp);
    /// returns true if tracing is enabled for the stand
    bool trace() const { return property(QStringLiteral("trace")).toBool(); }
    const QString &context() const { return mContextStr; }

    void reload(bool force=false); // fetch new data from the forest stand
    // general properties
    int id() const {return mId; }
    const FMUnit *unit() const { return mUnit; }
    Activity::Phase phase() const { return mPhase; }
    int standType() const { return mStandType; }
    FMSTP *stp() const {return mSTP; }
    int lastUpdate() const { return mLastUpdate; }
    int lastExecution() const { return mLastExecution; }
    /// total area of the stand (ha)
    double area() const;
    /// absolute age: years since the rotation has started
    double absoluteAge() const;
    /// total basal area (m2/ha)
    double basalArea() const {return mTotalBasalArea; }
    /// (average) age of the stand (weighted with basal area)
    double age() const {return mAge; }
    /// total standing volume (m3/ha) in the stand
    double volume() const {return mVolume; }
    /// number of trees of the stand (stems/ha) (>4m)
    double stems() const {return mStems; }
    /// scheduled harvest (planned harvest by activities, m3)
    double scheduledHarvest() const {return mScheduledHarvest; }
    /// total realized harvest (m3 on the full stand area)
    double totalHarvest() const { return mHarvested; }
    /// total disturbed timber volume, includes also disturbed trees *not* harvested
    double disturbedTimber() const { return mDisturbed; }

    /// mean annual increment (MAI), m3 timber/ha for the last decade
    double meanAnnualIncrement() const { return mMAIdecade / area(); }
    /// mean annual increment (MAI), m3 timber/ha for the full rotation period
    double meanAnnualIncrementTotal() const { return mMAItotal / area(); }

    bool readyForFinalHarvest() { return currentActivity()?(currentFlags().isFinalHarvest() && currentFlags().isScheduled()):false; }

    // specialized functions (invokable also from javascript)
    double basalArea(const QString &species_id) const;


    // actions
    /// main function
    bool execute(); ///< execute the current activity
    bool executeActivity(Activity *act); ///< execute activity given by "act".
    bool afterExecution(bool cancel = false);

    /// add a (simulated) harvest to the amount of planned harvest (used by the scheduling)
    void addScheduledHarvest(const double add_volume) {mScheduledHarvest += add_volume; }
    /// is called whenever a tree is removed (death, management, disturbance)
    void addTreeRemoval(Tree *tree, int reason);
    void resetHarvestCounter() { mHarvested = 0.; mDisturbed=0.; }

    /// sleep() pauses the evaluation/execution of management activities
    /// for 'years_to_sleep'.
    void sleep(int years_to_sleep);
    int sleepYears() const {return mYearsToWait; }

    /// calculate mean annual increment (m3/ha) and return total MAI.
    double calculateMAI();

    // return stand-specific flags
    ActivityFlags &flags(const int index)  {return mStandFlags[index]; }
    /// flags of currently active Activity
    ActivityFlags &currentFlags()  { return flags(mCurrentIndex); }
    /// get a pointer to the current activity; returns 0 if no activity is set.
    Activity *currentActivity() const { return mCurrentIndex>-1?mStandFlags[mCurrentIndex].activity():0; }

    // custom property storage
    static void clearAllProperties() { mStandPropertyStorage.clear(); }
    /// set a property value for the current stand with the name 'name'
    void setProperty(const QString &name, QJSValue value);
    /// retrieve the value of the property 'name'. Returns an empty QJSValue if the property is not defined.
    QJSValue property(const QString &name) const;

    // retrieve current state of the object
    QStringList info();
    friend class FOMEWrapper;
private:
    int mId; ///< the unique numeric ID of the stand
    FMUnit *mUnit; ///< management unit that
    FMSTP *mSTP; ///< the stand treatment program assigned to this stand
    Activity::Phase mPhase; ///< silvicultural phase
    int mStandType; ///< enumeration of stand (compositional)
    double mTotalBasalArea; ///< basal area of the stand
    double mAge; ///< average age (yrs) of the stand
    double mVolume; ///< standing volume (m3/ha) of the stand
    double mStems; ///< stems per ha (above 4m)
    double mScheduledHarvest; ///< harvest (m3) that is scheduled by activities
    double mHarvested; ///< m3 of timber volume that has been harvested
    double mDisturbed; ///< removed due to disturbance

    double mRemovedVolumeDecade; ///< removed volume of the decade (m3/ha)
    double mRemovedVolumeTotal; ///< removed volume of the rotation (m3/ha)

    double mLastMAIVolume; ///< safe the standing volume
    double mMAIdecade; ///< decadal mean annual increment (m3/ha*yr)
    double mMAItotal; ///< total (over the full rotation) mean annual increment (m3/ha*yr)


    int mRotationStartYear; ///< absolute year the current rotation has started
    int mYearsToWait; ///< variable indicates time to wait
    int mCurrentIndex; ///< the index of the current activity
    int mLastUpdate; ///< year of the last reload of data
    int mLastExecution; ///< year of the last execution of an activity

    void newRotatation(); ///< reset

    int nspecies() const  { return mSpeciesData.count(); }
    /// retrieve species-specific meta data by index (0: largest basal area share, up to nspecies()-1)
    SSpeciesStand &speciesData(const int index) {return mSpeciesData[index]; }
    SSpeciesStand &speciesData(const Species *species); ///< species-specific meta data by Species pointer
    // storage for stand meta data (species level)
    QVector<SSpeciesStand> mSpeciesData;
    // storage for stand-specific management properties
    QVector<ActivityFlags> mStandFlags;
    // additional property values for each stand
    QString mContextStr;
    static QHash<const FMStand*, QHash<QString, QJSValue> > mStandPropertyStorage;

    friend class StandObj;
};


} // namespace
#endif // FMSTAND_H
