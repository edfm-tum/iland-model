#include "amie_global.h"
#include <QJSValueIterator>

#include "activity.h"
#include "fmstand.h"
#include "fmstp.h"
#include "fomescript.h"
#include "fomewrapper.h"
#include "forestmanagementengine.h"

#include "expression.h"

// include derived activity types
#include "actgeneral.h"
#include "actscheduled.h"

namespace AMIE {

/***************************************************************************/
/***************************   Schedule  ***********************************/
/***************************************************************************/


void Schedule::setup(QJSValue &js_value)
{
    clear();
    if (js_value.isObject()) {
        tmin = FMSTP::valueFromJs(js_value, "min", "-1").toInt();
        tmax = FMSTP::valueFromJs(js_value, "max", "-1").toInt();
        topt = FMSTP::valueFromJs(js_value, "opt", "-1").toInt();
        tminrel = FMSTP::valueFromJs(js_value, "minRel", "-1").toNumber();
        tmaxrel = FMSTP::valueFromJs(js_value, "maxRel", "-1").toNumber();
        toptrel = FMSTP::valueFromJs(js_value, "optRel", "-1").toNumber();
        repeat_interval = FMSTP::valueFromJs(js_value, "repeatInterval", "1").toInt();
        // switches
        force_execution = FMSTP::boolValueFromJs(js_value, "force", false);
        repeat = FMSTP::boolValueFromJs(js_value, "repeat", false);
        absolute = FMSTP::boolValueFromJs(js_value, "absolute", false);
        if (!repeat) {
            if (tmin>-1 && tmax>-1 && topt==-1)
                topt = (tmax+tmin) / 2;
            if (tmin>-1 && tmax>-1 && topt>-1 && (topt<tmin || topt>tmax))
                throw IException(QString("Error in setting up schedule: 'opt' out of range: %1").arg(js_value.toString()));
            if (tminrel>-1 && tmaxrel>-1 && toptrel>-1 && (toptrel<tminrel || toptrel>tmaxrel))
                throw IException(QString("Error in setting up schedule: 'opt' out of range: %1").arg(js_value.toString()));
            if (tminrel*tmaxrel < 0. || tmin*tmax<0.)
                throw IException(QString("Error in setting up schedule: min and max required: %1").arg(js_value.toString()));

            if (topt==-1 && toptrel==-1.)
                throw IException(QString("Error in setting up schedule: neither 'opt' nor 'optRel' point can be derived in: %1").arg(js_value.toString()));
        }

    } else if (js_value.isNumber()) {
        topt = js_value.toNumber();
    } else {
        throw IException(QString("Error in setting up schedule/timing. Invalid javascript object: %1").arg(js_value.toString()));
    }
}

QString Schedule::dump() const
{
    if (repeat)
        return QString("schedule: Repeating every %1 years.").arg(repeat_interval);
    else
        return QString("schedule: tmin/topt/tmax %1/%2/%3 relative min/opt/max %4/%5/%6 force %7").arg(tmin).arg(topt).arg(tmax)
                .arg(tminrel).arg(toptrel).arg(tmaxrel).arg(force_execution);
}

double Schedule::value(const FMStand *stand)
{
    double U = stand->stp()->rotationLength();
    double current = stand->age();
    if (absolute)
        current = ForestManagementEngine::instance()->currentYear();

    double current_rel = current / U;
    // force execution: if age already higher than max, then always evaluate to 1.
    if (tmax>-1. && current > tmax && force_execution)
        return 1;
    if (tmaxrel>-1. && current_rel > tmaxrel && force_execution)
        return 1;

    if (tmin>-1. && current < tmin) return 0.;
    if (tmax>-1. && current > tmax) return 0.;
    if (tminrel>-1. && current_rel < tminrel) return 0.;
    if (tmaxrel>-1. && current_rel > tmaxrel) return 0.;

    // optimal time
    if (topt > -1. && fabs(current-topt) <= 0.5)
        return 1;

    if (tmin>-1. && tmax > -1.) {
        if (topt > -1.) {
            // linear interpolation
            if (current<=topt)
                return topt==tmin?1.:(current-tmin)/(topt-tmin);
            if (force_execution)
                return 1.; // keep the high probability.
            else
                return topt==tmax?1.:(tmax-current)/(tmax-topt); // decreasing probabilitiy again
        } else {
            return 1.; // no optimal time: everything between min and max is fine!
        }
    }
    // there is an optimal absoulte point in time defined, but not reached
    if (topt > -1)
        return 0.;

    // optimal time
    if (toptrel>-1. && fabs(current_rel-toptrel)*U <= 0.5)
        return 1.;

    // min/max relative time
    if (tminrel>-1. && tmaxrel>-1.) {
        if (toptrel > -1.) {
            // linear interpolation
            if (current_rel<=toptrel)
                return toptrel==tminrel?1.:(current_rel-tminrel)/(toptrel-tminrel);
            else
                return toptrel==tmaxrel?1.:(tmaxrel-current_rel)/(tmaxrel-toptrel);
        } else {
            return 1.; // no optimal time: everything between min and max is fine!
        }
    }
    // there is an optimal relative point in time defined, but not reached yet.
    if (toptrel>-1.)
        return 0.;

    qCDebug(abe) << "Schedule::value: unexpected combination. U" << U << "age" << current << ", schedule:" << this->dump();
    return 0.;
}

double Schedule::minValue(const double U) const
{
    if (tmin>-1) return tmin;
    if (tminrel>-1.) return tminrel * U; // assume a fixed U of 100yrs
    if (repeat) return -1.; // repeating executions are treated specially
    if (topt>-1) return topt;
    return toptrel * U;
}

double Schedule::maxValue(const double U) const
{
    if (tmax>-1) return tmax;
    if (tmaxrel>-1.) return tmaxrel * U; // assume a fixed U of 100yrs
    if (repeat) return -1.; // repeating executions are treated specially
    if (topt>-1) return topt;
    return toptrel * U;

}

/***************************************************************************/
/**************************     Events  ************************************/
/***************************************************************************/

void Events::clear()
{
    mEvents.clear();
}

void Events::setup(QJSValue &js_value, QStringList event_names)
{
    mInstance = js_value; // save the object that contains the events
    foreach (QString event, event_names) {
        QJSValue val = FMSTP::valueFromJs(js_value, event);
        if (val.isCallable()) {
            mEvents[event] = js_value; // save the event functions (and the name of the property that the function is assigned to)
        }
    }
}

QString Events::run(const QString event, FMStand *stand)
{
    if (mEvents.contains(event)) {
        if (stand)
            FomeScript::setExecutionContext(stand);
        QJSValue func = mEvents[event].property(event);
        QJSValue result;
        if (func.isCallable()) {
            result = func.callWithInstance(mInstance);
            if (FMSTP::verbose() || stand && stand->trace())
                qCDebug(abe) << (stand?stand->context():QString("<no stand>")) << "invoking javascript event" << event << " result: " << result.toString();
        }

        //qDebug() << "event called:" << event << "result:" << result.toString();
        if (result.isError()) {
            throw IException(QString("%3 Javascript error in event %1: %2").arg(event).arg(result.toString()).arg(stand?stand->context():"----"));
        }
        return result.toString();
    }
    return QString();
}

bool Events::hasEvent(const QString &event) const
{
    return mEvents.contains(event);
}

QString Events::dump()
{
    QString event_list = "Registered events: ";
    foreach (QString event, mEvents.keys())
        event_list.append(event).append(" ");
    return event_list;
}

/***************************************************************************/
/*************************  Constraints  ***********************************/
/***************************************************************************/

void Constraints::setup(QJSValue &js_value)
{
    mConstraints.clear();
    if ((js_value.isArray() || js_value.isObject()) && !js_value.isCallable()) {
        QJSValueIterator it(js_value);
        while (it.hasNext()) {
            it.next();
            mConstraints.append(constraint_item());
            constraint_item &item = mConstraints.last();
            item.setup(it.value());
        }
    } else {
        mConstraints.append(constraint_item());
        constraint_item &item = mConstraints.last();
        item.setup(js_value);

    }
}

double Constraints::evaluate(FMStand *stand)
{
    if (mConstraints.isEmpty())
        return 1.; // no constraints to evaluate
    double p;
    double p_min = 1;
    for (int i=0;i<mConstraints.count();++i) {
        p = mConstraints.at(i).evaluate(stand);
        if (p == 0.) {
            if (stand->trace())
                qCDebug(abe) << stand->context() << "constraint" << mConstraints.at(i).dump() << "did not pass.";
            return 0.; // one constraint failed
        } else {
            // save the lowest value...
            p_min = std::min(p, p_min);
        }
    }
    return p_min; // all constraints passed, return the lowest returned value...
}

QStringList Constraints::dump()
{
    QStringList info;
    for (int i=0;i<mConstraints.count();++i){
        info << QString("constraint: %1").arg(mConstraints[i].dump());
    }
    return info;
}


Constraints::constraint_item::~constraint_item()
{
    if (expr)
        delete expr;
}

void Constraints::constraint_item::setup(QJSValue &js_value)
{
    filter_type = ftInvalid;
    if (expr) delete expr;
    expr=0;

    if (js_value.isCallable()) {
        func = js_value;
        filter_type = ftJavascript;
        return;
    }
    if (js_value.isString()) {
        // we assume this is an expression

        QString exprstr = js_value.toString();
        // replace "." with "__" in variables (our Expression engine is
        // not able to cope with the "."-notation
        exprstr = exprstr.replace("activity.", "activity__");
        exprstr = exprstr.replace("stand.", "stand__");
        exprstr = exprstr.replace("site.", "site__");
        // add ....
        expr = new Expression(exprstr);
        filter_type = ftExpression;
        return;

    }
}

bool Constraints::constraint_item::evaluate(FMStand *stand) const
{
    switch (filter_type) {
    case ftInvalid: return true; // message?
    case ftExpression: {
            FOMEWrapper wrapper(stand);
            double result;
            try {
            result = expr->calculate(wrapper);
            } catch (IException &e) {
                // throw a nicely formatted error message
                e.add(QString("in filter (expr: '%2') for stand %1.").
                              arg(stand->id()).
                              arg(expr->expression()) );
                throw;
            }

            if (FMSTP::verbose())
                qCDebug(abe) << stand->context() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << stand->id() << ":" << result;
            return result > 0.;

        }
    case ftJavascript: {
        // call javascript function
        // provide the execution context
        FomeScript::setExecutionContext(stand);
        QJSValue result = const_cast<QJSValue&>(func).call();
        if (result.isError()) {
            throw IException(QString("Erron in evaluating constraint  (JS) for stand %1: %2").
                             arg(stand->id()).
                             arg(result.toString()));
        }
        if (FMSTP::verbose())
            qCDebug(abe) << "evaluate constraint (JS) for stand" << stand->id() << ":" << result.toString();
        return result.toBool();

    }

    }
    return true;
}

QString Constraints::constraint_item::dump() const
{
    switch (filter_type){
    case ftInvalid: return "Invalid";
    case ftExpression: return expr->expression();
    case ftJavascript: return func.toString();
    default: return "invalid filter type!";
    }
}



/***************************************************************************/
/***************************  Activity  ************************************/
/***************************************************************************/

Activity::Activity(const FMSTP *parent)
{
    mProgram = parent;
    mIndex = 0;
    mBaseActivity = ActivityFlags(this);
    mBaseActivity.setActive(true);
    mBaseActivity.setEnabled(true);
}

Activity::~Activity()
{

}

Activity *Activity::createActivity(const QString &type, FMSTP *stp)
{
    Activity *act = 0;

    if (type=="general")
        act = new ActGeneral(stp);

    if (type=="scheduled")
        act = new ActScheduled(stp);

    if (!act) {
        throw IException(QString("Error: the activity type '%1' is not a valid type.").arg(type));
    }

    return act;
}

QString Activity::type() const
{
    return "base";
}

void Activity::setup(QJSValue value)
{
    mSchedule.setup(FMSTP::valueFromJs(value, "schedule", "", "setup activity"));
    if (FMSTP::verbose())
        qCDebug(abeSetup) << mSchedule.dump();

    // setup of events
    mEvents.clear();
    mEvents.setup(value, QStringList() << "onCreate" << "onEnter" << "onExit" << "onExecuted" << "onCancel");
    if (FMSTP::verbose())
        qCDebug(abeSetup) << "Events: " << mEvents.dump();

    // setup of constraints
    QJSValue constraints = FMSTP::valueFromJs(value, "constraint");
    if (!constraints.isUndefined())
        mConstraints.setup(constraints);

}

double Activity::scheduleProbability(FMStand *stand)
{
    // return a value between 0 and 1
    return schedule().value(stand);
}

double Activity::execeuteProbability(FMStand *stand)
{
    // check the standard constraints and return true when all constraints are fulfilled (or no constraints set)
    return constraints().evaluate(stand);
}

bool Activity::execute(FMStand *stand)
{
    // execute the "onExecute" event
    events().run(QStringLiteral("onExecute"), stand);
    return true;
}

bool Activity::evaluate(FMStand *stand)
{
    // execute the "onExecute" event
    events().run(QStringLiteral("onEvaluate"), stand);
    return true;
}

QStringList Activity::info()
{
    QStringList lines;
    lines << QString("Activity '%1': type '%2'").arg(name()).arg(type());
    lines << "Events" << "-" << events().dump() << "/-";
    lines << "Schedule" << "-" << schedule().dump() << "/-";
    lines << "Constraints" << "-" << constraints().dump() << "/-";
    return lines;
}


ActivityFlags &Activity::standFlags(FMStand *stand)
{
    // use the base data item if no specific stand is provided
    if (!stand)
        return mBaseActivity;

    // return the flags associated with the specific stand
    return stand->flags(mIndex);
}


} // namespace

/*
 *
 *
 * header file:
 *
 *

/// Activity encapsulates an individual forest management activity.
/// Activities are stored and organized in the silvicultural KnowledgeBase.
class ActivityOld
{
public:
    ActivityOld();
    ~ActivityOld();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    // general properties
    QString name() const { return mJS.property("name").toString(); }
    QString description() const { return mJS.property("description").toString(); }
    // properties
    double knowledge() const {return mKnowledge; }
    double economy() const {return mEconomy; }
    double experimentation() const {return mExperimentation; }
    Phase phase() const { return mPhase; }
    // actions
    double evaluate(const FMStand *stand) const;
    // functions
    /// load definition of the Activity from an Javascript Object (value).
    bool setupFromJavascript(QJSValue &value, const QString &variable_name);
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode
private:
    bool addFilter(QJSValue &js_value, const QString js_name); ///< add a filter from the JS
    static bool mVerbose; ///< debug mode
    QJSValue mJS; ///< holds the javascript representation of the activity

    // properties of activities
    double mKnowledge;
    double mEconomy;
    double mExperimentation;
    Phase mPhase;

    // benchmarking
    int mJSEvaluations;
    int mExprEvaluations;

    // filter items
    struct filter_item {
        filter_item(): filter_type(ftInvalid), expression(0), value(0) {}
        filter_item(const filter_item &item); // copy constructor
        ~filter_item();
        // action
        double evaluate(const ActivityOld *act, const FMStand* stand) const;

        /// set the filter from javascript
        void set(QJSValue &js_value);
        QString toString(); ///< for debugging

        // properties
        enum { ftInvalid, ftConstant, ftExpression, ftJavascript} filter_type;
        Expression *expression;
        double value;
        QJSValue func;
        QString name;
    };
    QVector<filter_item> mFilters;


};

// ****** cpp file ****
#include "exception.h"
#include "expression.h"
#include "fomewrapper.h"
#include "fomescript.h"

bool ActivityOld::mVerbose = false;

/** @class Activity
 ** /

ActivityOld::ActivityOld()
{
    mPhase = Invalid;
    mEconomy = -1;
    mKnowledge = -1;
    mExperimentation = -1;
    // benchmarking
    mExprEvaluations = 0;
    mJSEvaluations = 0;
}

ActivityOld::~ActivityOld()
{
    qDebug() << "delete activity: calls JS: " << mJSEvaluations << " calls Expr: " << mExprEvaluations;
}

/// calculate the probability that this activity should be executed for the given stand
double ActivityOld::evaluate(const FMStand *stand) const
{
    // (1) check the silvicultural phase
    if (stand->phase() != mPhase)
        return 0.;

    // check the other filters
    double result = 1.;
    foreach(const filter_item &filter, mFilters) {
        double this_filter = filter.evaluate(this, stand);
        result = result * this_filter;
        if (result == 0.) {
            return 0.; // if the filter is 0, nothing happens
        }
    }

    return result;

}


/// setup the properties of the activity by scanning
/// the Javascript object
bool ActivityOld::setupFromJavascript(QJSValue &value, const QString &variable_name)
{
    mJS = value;
    if (verbose()) qDebug() << value.property("name").toString();
    // check for required properties
    if (value.property("name").isUndefined()) throw IException("'name' missing in " + variable_name);
    if (value.property("description").isUndefined()) throw IException("'description' missing in " + variable_name);

    // load properties
    QJSValue props = value.property("properties");
    mKnowledge = props.property("knowledge").toNumber();
    mEconomy = props.property("economy").toNumber();
    mExperimentation = props.property("experimentation").toNumber();
    QString phase = props.property("phase").toString();
    mPhase = Invalid;
    if (phase=="T") mPhase = Tending;
    if (phase=="H") mPhase = Thinning;
    if (phase=="R") mPhase = Regeneration;
    if (mPhase==Invalid)
        throw IException(QString("Invalid activity group '%1' (allowed: T, H, R)").arg(phase));

    // extract filter rules
    QJSValueIterator it(value.property("suitability"));
    while (it.hasNext()) {
        it.next();
        // extract filter...
        addFilter(it.value(), it.name());
    }
    return true;
}

bool ActivityOld::addFilter(QJSValue &js_value, const QString js_name)
{
    filter_item item;
    item.set(js_value);
    item.name = js_name;
    if (item.filter_type != filter_item::ftInvalid) {
        if (verbose())
            qDebug() << "added filter value" << item.toString();
        mFilters.append(item);
        return true;
    }
    qDebug() << "unable to add filter value:" << js_value.toString();
    return false;
}

// **** Filter ****

ActivityOld::filter_item::filter_item(const ActivityOld::filter_item &item):
    filter_type(ftInvalid), expression(0), value(0)
{
    filter_type = item.filter_type;
    name = item.name;
    switch (item.filter_type) {
    case ftConstant: value = item.value; break;
    case ftExpression: expression = new Expression(item.expression->expression()); break;
    case ftJavascript: func = item.func; break;
    }
}

ActivityOld::filter_item::~filter_item()
{
    if (expression) delete expression;
}

// main function for evaluating filters
double ActivityOld::filter_item::evaluate(const ActivityOld *act, const FMStand *stand) const
{
    if (filter_type == ftConstant)
        return value;

    if (filter_type == ftExpression) {
        FOMEWrapper wrapper(stand);
        double result;
        try {
        result = expression->calculate(wrapper);
        } catch (IException &e) {
            // throw a nicely formatted error message
            e.add(QString("in filter '%1' (expr: '%4') for activity '%2' for stand %3.").arg(name).
                          arg(act->name()).
                          arg(stand->id()).
                          arg(expression->expression()) );
            throw;

        }

        if (ActivityOld::verbose())
            qDebug() << "evaluate filter " << name << "(expr:" << expression->expression() << ") for stand" << stand->id() << ":" << result;
        const_cast<ActivityOld*>(act)->mExprEvaluations++;
        return result;
    }
    if (filter_type==ftJavascript) {
        // call javascript function
        // provide the execution context
        FomeScript::setExecutionContext(stand, act);
        QJSValue result = const_cast<QJSValue&>(func).call();
        if (result.isError()) {
            throw IException(QString("Erron in evaluating filter '%1' (JS) for activity '%2' for stand %3: %4").arg(name).
                             arg(act->name()).
                             arg(stand->id()).
                             arg(result.toString()));
        }
        if (ActivityOld::verbose())
            qDebug() << "evaluate filter " << name << "(JS:" << act->name() << ") for stand" << stand->id() << ":" << result.toString();
        const_cast<ActivityOld*>(act)->mJSEvaluations++;
        return result.toNumber();

    }
    return 0;
}

/// extracts the value from a javascript object and
/// stores it in a C++ structure.
void ActivityOld::filter_item::set(QJSValue &js_value)
{
    if (js_value.isNumber()) {
        value = js_value.toNumber();
        filter_type = ftConstant;
        return;
    }
    if (js_value.isString()) {
        // we assume this is an expression
        if (expression) delete expression;
        QString expr = js_value.toString();
        // replace "." with "__" in variables (our Expression engine is
        // not able to cope with the "."-notation
        expr = expr.replace("activity.", "activity__");
        expr = expr.replace("stand.", "stand__");
        expr = expr.replace("site.", "site__");
        // add ....
        expression = new Expression(expr);
        filter_type = ftExpression;
        return;
    }
    if (js_value.isCallable()) {
        func = js_value;
        filter_type = ftJavascript;
        return;
    }

    filter_type = ftInvalid;
}

// return current value of a filter
QString ActivityOld::filter_item::toString()
{
    switch (filter_type) {
    case ftInvalid: return "invalid item";
    case ftConstant: return QString("(const) %1").arg(value);
    case ftExpression: return QString("(expr) %1").arg(expression->expression());
    case ftJavascript: return QString("(js) %1").arg(func.toString());
    }
    return "Activity::filter_item::toString() unknown filter_type";
}


Activity::Activity(const FMSTP *parent)
{
    mProgram = parent;
}
*/

