#include "fome_global.h"
#include <QJSValueIterator>

#include "activity.h"
#include "fmstand.h"

#include "exception.h"
#include "expression.h"
#include "fomewrapper.h"
#include "fomescript.h"

bool Activity::mVerbose = false;

/** @class Activity
 **/

Activity::Activity()
{
    mPhase = Invalid;
    mEconomy = -1;
    mKnowledge = -1;
    mExperimentation = -1;
}

/// calculate the probability that this activity should be executed for the given stand
double Activity::evaluate(const FMStand *stand) const
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
bool Activity::setupFromJavascript(QJSValue &value, const QString &variable_name)
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

bool Activity::addFilter(QJSValue &js_value, const QString js_name)
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

Activity::filter_item::filter_item(const Activity::filter_item &item):
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

Activity::filter_item::~filter_item()
{
    if (expression) delete expression;
}

// main function for evaluating filters
double Activity::filter_item::evaluate(const Activity *act, const FMStand *stand) const
{
    if (filter_type == ftConstant)
        return value;

    if (filter_type == ftExpression) {
        FOMEWrapper wrapper(act, stand);
        double result;
        try {
        result = expression->calculate(wrapper);
        } catch (IException &e) {
            // throw a nicely formatted error message
            e.add(QString("in filter '%1' (expr: '%5') for activity '%2' for stand %3: %4").arg(name).
                          arg(act->name()).
                          arg(stand->id()).
                          arg(e.message()).
                          arg(expression->expression()) );
            throw;

        }

        if (Activity::verbose())
            qDebug() << "evaluate filter " << name << "(expr:" << expression->expression() << ") for stand" << stand->id() << ":" << result;
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
        return result.toNumber();

    }
    return 0;
}

/// extracts the value from a javascript object and
/// stores it in a C++ structure.
void Activity::filter_item::set(QJSValue &js_value)
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
QString Activity::filter_item::toString()
{
    switch (filter_type) {
    case ftInvalid: return "invalid item";
    case ftConstant: return QString("(const) %1").arg(value);
    case ftExpression: return QString("(expr) %1").arg(expression->expression());
    case ftJavascript: return QString("(js) %1").arg(func.toString());
    }
    return "Activity::filter_item::toString() unknown filter_type";
}
