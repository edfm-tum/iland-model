#ifndef FMSSTAND_H
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
    // property values for each stand
    static QHash<FMStand*, QHash<QString, QJSValue> > mStandPropertyStorage;
};

#endif // FMSTAND_H
