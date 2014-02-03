#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <QJSValue>
#include <QVector>

class Expression; // forward

namespace AMIE {


class FMStand;
class FMSTP;


class Schedule {
public:
    // setup and life cycle
    Schedule()  {}
    Schedule(QJSValue &js_value) { clear(); setup(js_value); }
    void clear() { tmin=tmax=topt=-1; tminrel=tmaxrel=toptrel=-1.; force_execution=false; repeat_interval=-1; repeat=false; absolute=false; }
    void setup(QJSValue &js_value);
    QString dump() const;
    // functions
    /// value() evaluates the schedule for the given 'stand'
    double value(const FMStand *stand);
    /// gives (fixed) earliest possible execution time
    double minValue() const;
    // some stuffs
    int tmin; int tmax; int topt;
    double tminrel; double tmaxrel; double toptrel;
    bool force_execution;
    // repeating
    int repeat_interval;
    bool repeat;
    bool absolute;
};

class Events {
public:
    Events() {}
    /// clear the list of events
    void clear();
    /// setup events from the javascript object
    void setup(QJSValue &js_value, QStringList event_names);
    ///< execute javascript event /if registered) in the context of the forest stand 'stand'.
    QString run(const QString event, const FMStand *stand);
    QString dump(); ///< prints some debug info
private:
    QMap<QString, QJSValue> mEvents;
};

class Constraints {
public:
    Constraints() {}
    void setup(QJSValue &js_value); ///< setup from javascript
    bool evaluate(const FMStand *stand); ///< run the constraints
    QStringList dump(); ///< prints some debug info
private:
    struct constraint_item {
        constraint_item(): filter_type(ftInvalid), expr(0) {}
        ~constraint_item();
        void setup(QJSValue &js_value);
        bool evaluate(const FMStand *stand) const;
        QString dump() const;

        enum { ftInvalid, ftExpression, ftJavascript} filter_type;
        Expression *expr;
        QJSValue func;
    };

    QList<constraint_item> mConstraints;
};

class Activity; //forward
/** Activity meta data (enabled, active, ...) that need to be stored
    per stand. */
class ActivityFlags
{
public:
    ActivityFlags(): mActivity(0), mFlags(0) {}
    ActivityFlags(Activity *act): mActivity(act), mFlags(0) {}
    bool active() const {return flag(Active); }
    bool enabled() const {return flag(Enabled);}
    bool isRepeating() const {return flag(Repeater);}
    void setActive(const bool active) { setFlag(Active, active); }
    void setEnabled(const bool enabled) { setFlag(Enabled, enabled); }
    void setIsRepeating(const bool repeat) { setFlag(Repeater, repeat); }
private:
    enum Flags { Active=1, Enabled=2, Repeater=4 }; ///< (binary coded)  flags
    bool flag(const ActivityFlags::Flags flag) const { return mFlags & flag; }
    void setFlag(const ActivityFlags::Flags flag, const bool value) { if (value) mFlags |= flag; else mFlags &= (flag ^ 0xffffff );}
    Activity *mActivity; ///< link to activity
    int mFlags;
};
Q_DECLARE_TYPEINFO(ActivityFlags, Q_PRIMITIVE_TYPE); // declare as POD structure to allow more efficient copying


/// Activity is the base class for management activities
class Activity
{
public:
    Activity(const FMSTP *parent);
    ~Activity();
    enum Phase { Invalid, Tending, Thinning, Regeneration, All };
    const FMSTP *program() const { return mProgram; }
    virtual QString type() const;
    QString name() const {return mName; }
    int index() const { return mIndex; }
    int earlistSchedule() const {return mSchedule.minValue(); }
    // main actions
    /// setup of the activity (events, schedule, constraints). additional setup in derived classes.
    virtual void setup(QJSValue value);
    /// executes the action (usually defined in derived classes) using the context of 'stand'.
    virtual bool execute(FMStand *stand);
    /// dumps some information for debugging
    virtual QStringList info();
protected:
    Schedule &schedule()  { return mSchedule; }
    Constraints &constraints()  { return mConstraints; }
    Events &events()  { return mEvents; }
    ActivityFlags &standFlags(FMStand *stand);
private:
    void setIndex(const int index) { mIndex = index; } // used during setup
    void setName(const QString &name) { mName = name; }
    int mIndex;
    QString mName; ///< the name of the activity;
    const FMSTP *mProgram; // link to the management programme the activity is part of
    Schedule mSchedule; // timing of activity
    Constraints mConstraints; // constraining factors
    Events mEvents; // action handlers such as "onExecute"
    ActivityFlags mBaseActivity; // base properties of the activity (that can be changed for each stand)
    friend class FMSTP; // allow access of STP class to internals
};

} // namespace
#endif // ACTIVITY_H
