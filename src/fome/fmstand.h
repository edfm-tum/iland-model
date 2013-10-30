#ifndef FMSTAND_H
#define FMSTAND_H

#include <QHash>
#include <QJSValue>

#include "activity.h"

class FOMEWrapper; // forward
class FMUnit; // forward

/** FMStand encapsulates a forest stand for the forest management engine.
 *  The spatial coverage is defined by a "stand grid".
 * */

class FMStand
{
public:
    FMStand(FMUnit *unit);
    // general properties
    int id() const {return mId; }
    const FMUnit *unit() const { return mUnit; }
    Activity::Phase phase() const { return mPhase; }
    int standType() const { return mStandType; }
    //
    /// total basal area (m2/ha)
    double basalArea() const {return mBasalArea; }
    /// (average) age of the stand
    double age() const {return mAge; }
    /// total standing volume (m3/ha) in the stand
    double volume() const {return mVolume; }

    // specialized functions (invokable also from javascript)
    double basalArea(const QString &species_id);
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
    Activity::Phase mPhase; ///< silvicultural phase
    int mStandType; ///< enumeration of stand (compositional)
    double mBasalArea; ///< basal area of the stand
    double mAge; ///< average age (yrs) of the stand
    double mVolume; ///< standing volume (m3/ha) of the stand
    // property values for each stand
    static QHash<FMStand*, QHash<QString, QJSValue> > mStandPropertyStorage;
};

#endif // FMSTAND_H
