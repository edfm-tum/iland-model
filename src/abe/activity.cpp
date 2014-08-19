#include "abe_global.h"
#include <QJSValueIterator>

#include "activity.h"
#include "fmstand.h"
#include "fmstp.h"
#include "fomescript.h"
#include "fomewrapper.h"
#include "forestmanagementengine.h"

#include "expression.h"
#include "debugtimer.h"


// include derived activity types
#include "actgeneral.h"
#include "actscheduled.h"
#include "actplanting.h"
#include "actthinning.h"

namespace ABE {

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
        return QString("schedule: tmin/topt/tmax %1/%2/%3\nrelative: min/opt/max %4/%5/%6\nforce: %7").arg(tmin).arg(topt).arg(tmax)
                .arg(tminrel).arg(toptrel).arg(tmaxrel).arg(force_execution);
}

double Schedule::value(const FMStand *stand)
{
    double U = stand->stp()->rotationLength();
    double current;
    double current_year = ForestManagementEngine::instance()->currentYear();
    // absolute age: years since the start of the rotation
    current = stand->absoluteAge();

    if (absolute)
        current = current_year;

    double current_rel = current / U;
    if (repeat) {
        // handle special case of repeating activities.
        // we execute the repeating activity if repeatInterval years elapsed
        // since the last execution.
        if (int(current_year) % repeat_interval == 0)
            return 1.; // yes, execute this year
        else
            return 0.; // do not execute this year.

    }
    // force execution: if age already higher than max, then always evaluate to 1.
    if (tmax>-1. && current >= tmax && force_execution)
        return 1;
    if (tmaxrel>-1. && current_rel >= tmaxrel && force_execution)
        return 1;

    if (tmin>-1. && current < tmin) return 0.;
    if (tmax>-1. && current > tmax) return -1.; // expired
    if (tminrel>-1. && current_rel < tminrel) return 0.;
    if (tmaxrel>-1. && current_rel > tmaxrel) return -1.; // expired

    // optimal time
    if (topt > -1. && fabs(current-topt) <= 0.5)
        return 1;
    if (topt > -1. && current > topt) {
        if (force_execution)
            return 1.;
        else
            return -1.; // expired
    }

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
    if (repeat) return 100000000.;
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

double Schedule::optimalValue(const double U) const
{
    if (topt>-1) return topt;
    if (toptrel>-1) return toptrel*U;
    if (tmin>-1 && tmax>-1) return (tmax+tmin)/2.;
    if (tminrel>-1 && tmaxrel>-1) return (tmaxrel+tminrel)/2. * U;
    if (force_execution) return tmax;
    return toptrel*U;
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
            DebugTimer t("ABE:JSEvents:run");

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
            if (it.name()==QStringLiteral("length"))
                continue;
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
                result = expr->execute(0, &wrapper); // using execute, we're in strict mode, i.e. wrong variables are reported.
                //result = expr->calculate(wrapper);
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

    if (type=="planting")
        act = new ActPlanting(stp);

    if (type=="salvage")
        act = new ActSalvage(stp);

    if (type=="thinning")
        act = new ActThinning(stp);

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
    mEvents.setup(value, QStringList() << "onCreate" << "onSetup" << "onEnter" << "onExit" << "onExecute" << "onExecuted" << "onCancel");
    if (FMSTP::verbose())
        qCDebug(abeSetup) << "Events: " << mEvents.dump();

    // setup of constraints
    QJSValue constraints = FMSTP::valueFromJs(value, "constraint");
    if (!constraints.isUndefined())
        mConstraints.setup(constraints);

}

double Activity::scheduleProbability(FMStand *stand)
{
    // return a value between 0 and 1; return -1 if the activity is expired.
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
    // execute the "onEvaluate" event: the execution is canceled, if the function returns false.
    bool cancel = events().run(QStringLiteral("onEvaluate"), stand)==QStringLiteral("false");
    return !cancel;
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

