#include "fome_global.h"
#include <QJSValueIterator>

#include "activity.h"
#include "exception.h"
#include "expression.h"

bool Activity::mVerbose = false;

/** @class Activity
 **/

Activity::Activity()
{
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

    // extract properties
    QJSValue props = value.property("properties");
    QJSValueIterator it(props);
    if (props.isUndefined()) throw IException("'properties' missing in " + variable_name);
    while (it.hasNext()) {
        it.next();
        // load properties
        mKnowledge = it.value().property("knowledge").toNumber();
        mEconomy = it.value().property("economy").toNumber();
    }

    // extract filter rules
    QJSValueIterator it_filter(value.property("suitability"));
    while (it_filter.hasNext()) {
        it_filter.next();
        // extract filter...
        addFilter(it_filter.value());
    }
    return true;
}

bool Activity::addFilter(QJSValue &js_value)
{
    filter_item item;
    item.set(js_value);
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
        expression = new Expression(js_value.toString());
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
