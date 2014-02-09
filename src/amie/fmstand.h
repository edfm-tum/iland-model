#ifndef FMSTAND_H
#define FMSTAND_H

#include <QHash>
#include <QJSValue>

#include "activity.h"

class Species; // forward (iLand species)

namespace AMIE {

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

    void reload(); // fetch new data from the forest stand
    // general properties
    int id() const {return mId; }
    const FMUnit *unit() const { return mUnit; }
    Activity::Phase phase() const { return mPhase; }
    int standType() const { return mStandType; }
    FMSTP *stp() const {return mSTP; }
    //
    /// total basal area (m2/ha)
    double basalArea() const {return mTotalBasalArea; }
    /// (average) age of the stand
    double age() const {return mAge; }
    /// total standing volume (m3/ha) in the stand
    double volume() const {return mVolume; }

    // specialized functions (invokable also from javascript)
    double basalArea(const QString &species_id) const;


    // actions
    /// main function
    bool execute();
    bool afterExecution();

    /// sleep() pauses the evaluation/execution of management activities
    /// for 'years_to_sleep'.
    void sleep(int years_to_sleep);
    int sleepYears() const {return mYearsToWait; }

    // return stand-specific flags
    ActivityFlags &flags(const int index)  {return mStandFlags[index]; }
    // custom property storage
    static void clearAllProperties() { mStandPropertyStorage.clear(); }
    /// set a property value for the current stand with the name 'name'
    void setProperty(const QString &name, QJSValue value);
    /// retrieve the value of the property 'name'. Returns an empty QJSValue if the property is not defined.
    QJSValue property(const QString &name);

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

    int mYearsToWait; ///< variable indicates time to wait
    int mCurrentIndex; ///< the index of the current activity

    /// flags of currently active Activity
    ActivityFlags &currentFlags()  { return flags(mCurrentIndex); }
    Activity *currentActivity() const { return mStandFlags[mCurrentIndex].activity(); }
    int nspecies() const  { return mSpeciesData.count(); }
    /// retrieve species-specific meta data by index (0: largest basal area share, up to nspecies()-1)
    SSpeciesStand &speciesData(const int index) {return mSpeciesData[index]; }
    SSpeciesStand &speciesData(const Species *species); ///< species-specific meta data by Species pointer
    // storage for stand meta data (species level)
    QVector<SSpeciesStand> mSpeciesData;
    // storage for stand-specific management properties
    QVector<ActivityFlags> mStandFlags;
    // additional property values for each stand
    static QHash<FMStand*, QHash<QString, QJSValue> > mStandPropertyStorage;

    friend class StandObj;
};


} // namespace
#endif // FMSTAND_H
