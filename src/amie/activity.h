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
    double minValue(const double U=100.) const;
    /// returns the latest possible execution time
    double maxValue(const double U=100.) const;
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
    QString run(const QString event, FMStand *stand);
    QString dump(); ///< prints some debug info
private:
    QJSValue mInstance; ///< object holding the events
    QMap<QString, QJSValue> mEvents; ///< list of event names and javascript functions
};

class Constraints {
public:
    Constraints() {}
    void setup(QJSValue &js_value); ///< setup from javascript
    double evaluate(FMStand *stand); ///< run the constraints
    QStringList dump(); ///< prints some debug info
private:
    struct constraint_item {
        constraint_item(): filter_type(ftInvalid), expr(0) {}
        ~constraint_item();
        void setup(QJSValue &js_value);
        bool evaluate(FMStand *stand) const;
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
    Activity *activity() const {return mActivity;
                               }
    bool active() const {return flag(Active); }
    bool enabled() const {return flag(Enabled);}
    bool isRepeating() const {return flag(Repeater);}
    bool isPending() const {return flag(Pending); }
    bool isForcedNext() const {return flag(ExecuteNext); }
    bool isFinalHarvest() const {return flag(FinalHarvest); }
    bool isExecuteImmediate() const {return flag(ExecuteImmediate); }
    void setActive(const bool active) { setFlag(Active, active); }
    void setEnabled(const bool enabled) { setFlag(Enabled, enabled); }
    void setIsRepeating(const bool repeat) { setFlag(Repeater, repeat); }
    void setIsPending(const bool pending) { setFlag(Pending, pending); }
    void setForceNext(const bool isnext) { setFlag(ExecuteNext, isnext); }
    void setFinalHarvest(const bool isfinal) { setFlag(FinalHarvest, isfinal); }
    void setExecuteImmediate(const bool doexec) { setFlag(ExecuteImmediate, doexec);}

private:
    /// (binary coded)  flags
    enum Flags { Active=1,  // if true, the activity is currently the active activity
                 Enabled=2,  // if false, the activity can not be executed
                 Repeater=4, // if true, the activity is executed
                 ExecuteNext=8, // this activity should be executed next (kind of "goto"
                 ExecuteImmediate=16, // should be executed immediately by the scheduler (e.g. required sanitary cuttings)
                 Pending=32,  // the activity is currently in the scheduling algorithm
                 FinalHarvest=64  // the management of the activity is a "endnutzung" (compared to "vornutzung")
                 };
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
    int earlistSchedule(const double U=100.) const {return mSchedule.minValue(U); }
    int latestSchedule(const double U=100.) const { return mSchedule.maxValue(U); }
    // main actions
    /// setup of the activity (events, schedule, constraints). additional setup in derived classes.
    virtual void setup(QJSValue value);
    /// returns a value > 0 if the activity coult be scheduled now
    virtual double scheduleProbability(FMStand *stand);
    /// returns a probability for the activity to be executed (ie all constraints are fulfilled)
    /// return value is 0 if the activity can not be executed (maximum result is 1)
    virtual double execeuteProbability(FMStand *stand);
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
    friend class FMStand; // allow access of the activity class (e.g for events)
};

} // namespace
#endif // ACTIVITY_H
