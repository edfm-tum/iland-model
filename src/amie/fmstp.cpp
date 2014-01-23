#include "global.h"
#include "fome_global.h"
#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fomewrapper.h"

#include "expression.h"

// static values
bool FMSTP::mVerbose = false;

FMSTP::FMSTP()
{
}

// read the setting from the setup-javascript object
void FMSTP::setup(QJSValue &js_value)
{
    QJSValue thinning = valueFromJs(js_value, "thinning", "", "setup STP");
    Schedule thinning_timing;
    thinning_timing.setup(valueFromJs(thinning, "timing", "", "setup thinning"));
    qDebug() << thinning_timing.dump();

    // setup of events
    Events thinning_events;
    thinning_events.setup(thinning, QStringList() << "onEnter" << "onExit" << "onHarvest");
    qDebug() << "Events of thinning: " << thinning_events.dump();

    // setup of constraints
    Constraints thinning_constraints;
    QJSValue constraints = valueFromJs(thinning, "constraint", "");
    if (!constraints.isUndefined())
        thinning_constraints.setup(constraints);

    // tests
    FMStand *test = new FMStand(0,1);
    thinning_constraints.evaluate(test);
    for (int i=0;i<100;++i) {
        test->reload();
        qDebug()<< "year" << test->age() << "result: " << thinning_timing.value(test);
    }
    // testing events
    qDebug() << "onEnter: " << thinning_events.run("onEnter", test);
    qDebug() << "onExit: " << thinning_events.run("onExit", test);

    delete test;
}

// run the management for the forest stand 'stand'
bool FMSTP::execute(FMStand &stand)
{
    switch (stand.phase()) {
        case Regeneration: break;
    }
    return true;
}

QJSValue FMSTP::valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage)
{
   if (!js_value.hasOwnProperty(key)) {
       if (!errorMessage.isEmpty())
           throw IException(QString("Error: required key '%1' not found. In: %2").arg(key).arg(errorMessage));
       else
           return default_value;
   }
   return js_value.property(key);
}

/***************************************************************************/
/***************************   Schedule  ***********************************/
/***************************************************************************/


void FMSTP::Schedule::setup(QJSValue &js_value)
{
    clear();
    if (js_value.isObject()) {
        tmin = valueFromJs(js_value, "min", "-1").toInt();
        tmax = valueFromJs(js_value, "max", "-1").toInt();
        topt = valueFromJs(js_value, "opt", "-1").toInt();
        tminrel = valueFromJs(js_value, "minRel", "-1").toNumber();
        tmaxrel = valueFromJs(js_value, "maxRel", "-1").toNumber();
        toptrel = valueFromJs(js_value, "optRel", "-1").toNumber();
        force_execution = valueFromJs(js_value, "force", "false").toBool();
        if (tmin>-1. && tmax>-1. && topt>-1. && (topt<tmin || topt>tmax))
            throw IException(QString("Error in setting up timing: topt out of scope: %1").arg(js_value.toString()));
        if (tminrel>-1. && tmaxrel>-1. && toptrel>-1. && (toptrel<tminrel || toptrel>tmaxrel))
            throw IException(QString("Error in setting up timing: topt out of scope: %1").arg(js_value.toString()));
        if (tminrel*tmaxrel < 0. || tmin*tmax<0.)
            throw IException(QString("Error in setting up timing: min and max required: %1").arg(js_value.toString()));

    } else if (js_value.isNumber()) {
        topt = js_value.toNumber();
    } else {
        throw IException(QString("Error in setting up schedule/timing. Invalid javascript object: %1").arg(js_value.toString()));
    }
}

QString FMSTP::Schedule::dump() const
{
    return QString("schedule. tmin/topt/tmax: %1/%2/%3 relative: min/opt/max: %4/%5/%6 force: %7").arg(tmin).arg(topt).arg(tmax)
            .arg(tminrel).arg(toptrel).arg(tmaxrel).arg(force_execution);
}

double FMSTP::Schedule::value(const FMStand *stand)
{
    double U = 100.; // todo: fix!
    double age = stand->age();
    // force execution: if age already higher than max, then always evaluate to 1.
    if (tmax>-1. && age > tmax && force_execution)
        return 1;
    if (tmaxrel>-1. && age/U > tmaxrel && force_execution)
        return 1;

    if (tmin>-1. && age < tmin) return 0.;
    if (tmax>-1. && age > tmax) return 0.;
    if (tminrel>-1. && age/U < tminrel) return 0.;
    if (tmaxrel>-1. && age/U > tmaxrel) return 0.;

    if (tmin>-1. && tmax > -1.) {
        if (topt > -1.) {
        // linear interpolation
            if (age<=topt)
                return topt==tmin?1.:(age-tmin)/(topt-tmin);
            else
                return topt==tmax?1.:(tmax-age)/(tmax-topt);
        } else {
            return 1.; // no optimal time: everything between min and max is fine!
        }
    }
    if (tminrel>-1. && tmaxrel>-1.) {
        if (toptrel > -1.) {
        // linear interpolation
            if (age<=toptrel)
                return toptrel==tminrel?1.:(age-tminrel)/(toptrel-tminrel);
            else
                return toptrel==tmaxrel?1.:(tmaxrel-age)/(tmaxrel-toptrel);
        } else {
            return 1.; // no optimal time: everything between min and max is fine!
        }
    }
    qDebug() << "Schedule::value: unexpected combination. U" << U << "age" << age << ", schedule:" << this->dump();
    return 0.;
}

/***************************************************************************/
/**************************     Events  ************************************/
/***************************************************************************/

void FMSTP::Events::setup(QJSValue &js_value, QStringList event_names)
{
    mEvents.clear();
    foreach (QString event, event_names) {
        QJSValue val = valueFromJs(js_value, event, "");
        if (val.isCallable())
            mEvents[event] = val;
    }
}

QString FMSTP::Events::run(const QString event, const FMStand *stand)
{
    if (mEvents.contains(event)) {
        FomeScript::setExecutionContext(stand);
        if (FMSTP::verbose())
            qDebug() << "running javascript event" << event;
        QJSValue result = mEvents[event].call();
        if (result.isError()) {
            throw IException(QString("Javascript error in event %1: %2").arg(event).arg(result.toString()));
        }
        return result.toString();
    }
    return QString();
}

QString FMSTP::Events::dump()
{
    QString event_list = "Registered events: ";
    foreach (QString event, mEvents.keys())
        event_list.append(event).append(" ");
    return event_list;
}

/***************************************************************************/
/*************************  Constraints  ***********************************/
/***************************************************************************/

void FMSTP::Constraints::setup(QJSValue &js_value)
{
    mConstraints.clear();
    if (js_value.isArray() || js_value.isObject()) {
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

bool FMSTP::Constraints::evaluate(const FMStand *stand)
{
    if (mConstraints.isEmpty())
        return true; // no constraints to evaluate
    for (int i=0;i<mConstraints.count();++i)
        if (!mConstraints.at(i).evaluate(stand))
            return false; // one constraint failed
    return true; // all constraints passed
}


FMSTP::Constraints::constraint_item::~constraint_item()
{
    if (expr)
        delete expr;
}

void FMSTP::Constraints::constraint_item::setup(QJSValue &js_value)
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

bool FMSTP::Constraints::constraint_item::evaluate(const FMStand *stand) const
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
                qDebug() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << stand->id() << ":" << result;
            return result;

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
            qDebug() << "evaluate constraint (JS) for stand" << stand->id() << ":" << result.toString();
        return result.toNumber();

    }

    }
    return true;
}
