#ifndef FOMESTP_H
#define FOMESTP_H

#include "fmstand.h"
#include "activity.h"

#include <QJSValue>
#include <QList>

class Expression; // forward

namespace AMIE {

/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */
class Activity; // forward

class FMSTP
{
public:
    FMSTP();
    ~FMSTP();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    const QString &name() const {return mName; }
    /// returns the (first) Activity with the name 'name', or 0 if the activity could not be found.
    Activity *activity(const QString &name) const;
    int activityIndex(Activity* act) { return mActivities.indexOf(act); }

    /// read the options from a javascript structure / object
    void setup(QJSValue &js_value, const QString name=QString());
    /// defaultFlags() is used to initalized the flags for indiv. forest stands
    QVector<ActivityFlags> defaultFlags() {return mActivityStand; }
    Events &events() { return mEvents; }

    /// rotation length (years)
    int rotationLength() const {return 100; } // TODO: fix



    /// main function that runs the current program for stand 'stand'
    bool execute(FMStand &stand);

    // helper functions
    void dumpInfo();
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode
    /// get a property of 'js_value' with the name 'key'. If 'errorMessage' is given, an error is thrown when the key does not exist.
    /// If key is not present and a 'default_value' is given, it is returned. Otherwise, an "undefined" JS value is returned.
    static QJSValue valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value=QString(), const QString &errorMessage=QString());
    static bool boolValueFromJs(const QJSValue &js_value, const QString &key, const bool default_bool_value, const QString &errorMessage=QString());

private:
    void internalSetup(QJSValue &js_value, int level=0);
    QString mName; ///< the name of the stand treatment program
    void setupActivity(QJSValue &js_value, const QString &name);
    void clear(); ///< remove all activites
    Events mEvents;
    static bool mVerbose; ///< debug mode
    QVector<Activity*> mActivities; ///< container for the activities of the STP
    QVector<ActivityFlags> mActivityStand; ///< base data for stand-specific STP info.
    QStringList mActivityNames;  ///< names of all available activities

};

} // namespace

#endif // FOMESTP_H
