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
    void clear() { tmin=tmax=topt=-1; tminrel=tmaxrel=toptrel=-1.; force_execution=false; }
    void setup(QJSValue &js_value);
    QString dump() const;
    // functions
    double value(const FMStand *stand);
    // some stuffs
    int tmin; int tmax; int topt;
    double tminrel; double tmaxrel; double toptrel;
    bool force_execution;
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


/// Activity is the base class for management activities
class Activity
{
public:
    Activity(const FMSTP *parent);
    ~Activity();
    enum Phase { Invalid, Tending, Thinning, Regeneration, All };
    const FMSTP *program() const { return mProgram; }
    virtual QString name() const;
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
private:
    const FMSTP *mProgram; // link to the management programme the activity is part of
    Schedule mSchedule; // timing of activity
    Constraints mConstraints; // constraining factors
    Events mEvents; // action handlers such as "onExecute"
};


} // namespace
#endif // ACTIVITY_H
