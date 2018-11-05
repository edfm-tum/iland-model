#include "bitecellscript.h"

#include "bitewrapper.h"
#include "expressionwrapper.h"
#include "biteengine.h"
#include "fmtreelist.h"

#include "debugtimer.h"

namespace BITE {
BiteCellScript::BiteCellScript(QObject *parent) : QObject(parent)
{

}

ABE::FMTreeList *BiteCellScript::trees()
{
    // now this is tricky:
    // let us return the tree list attached to the current thread:
    return BiteAgent::threadTreeList();
}

void BiteCellScript::reloadTrees()
{
    int n = mCell->loadTrees(trees());
    qCDebug(bite) << "reloaded trees for cell" << mCell->index() << ", N=" << n << " (treelist: " << trees();
}

// ***********************************************************
// *******************   DynamicExpression *******************
// ***********************************************************


DynamicExpression::~DynamicExpression()
{
    if (expr)
        delete expr;
}

void DynamicExpression::setup(const QJSValue &js_value, EWrapperType type)
{
    filter_type = ftInvalid;
    wrapper_type = type;
    if (expr) delete expr;
    expr=nullptr;

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
//        exprstr = exprstr.replace("activity.", "activity__");
//        exprstr = exprstr.replace("stand.", "stand__");
//        exprstr = exprstr.replace("site.", "site__");
//        exprstr = exprstr.replace("unit.", "unit__");
        // add ....
        expr = new Expression(exprstr);
        filter_type = ftExpression;
        return;

    }
}

bool DynamicExpression::evaluate(BiteCell *cell) const
{
    switch (filter_type) {
    case ftInvalid: return true; // message?
    case ftExpression: {
            BiteWrapper bitewrap(cell);
            double result;
            try {
                result = expr->execute(nullptr, &bitewrap); // using execute, we're in strict mode, i.e. wrong variables are reported.
                //result = expr->calculate(wrapper);
            } catch (IException &e) {
                // throw a nicely formatted error message
                e.add(QString("in filter (expr: '%2') for cell %1.").
                              arg(cell->index()).
                              arg(expr->expression()) );
                throw;
            }

//            if (FMSTP::verbose())
//                qCDebug(abe) << cell->context() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << cell->id() << ":" << result;
            return result > 0.;

        }
    case ftJavascript: {
        // call javascript function
        // provide the execution context
        // FomeScript::setExecutionContext(cell);
        QJSValue result = const_cast<QJSValue&>(func).call();
        if (result.isError()) {
            throw IException(QString("Erron in evaluating constraint  (JS) for cell %1: %2").
                             arg(cell->index()).
                             arg(result.toString()));
        }
//        if (FMSTP::verbose())
//            qCDebug(abe) << "evaluate constraint (JS) for stand" << cell->id() << ":" << result.toString();
        // convert boolean result to 1 - 0
        return result.toBool();
//        if (result.isBool())
//            return result.toBool()==true?1.:0;
//        else
//            return result.toNumber();
    }

    }
    return true;
}

bool DynamicExpression::evaluate(ABE::FMTreeList *treelist) const
{
    switch (filter_type) {
    case ftInvalid: return true; // message?
    case ftExpression: {
            TreeWrapper treewrap(nullptr);
            double result;
            try {
                result = expr->execute(nullptr, &treewrap); // using execute, we're in strict mode, i.e. wrong variables are reported.
                //result = expr->calculate(wrapper);
            } catch (IException &e) {
                // throw a nicely formatted error message
                e.add(QString("in filter (expr: '%2') for treelist (N=%1).").
                              arg(treelist->trees().size()).
                              arg(expr->expression()) );
                throw;
            }

//            if (FMSTP::verbose())
//                qCDebug(abe) << cell->context() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << cell->id() << ":" << result;
            return result > 0.;

        }
    case ftJavascript: {
        // call javascript function
        // provide the execution context
        // FomeScript::setExecutionContext(cell);
        QMutexLocker lock(BiteEngine::instance()->serializeJS());

        QJSValue result = const_cast<QJSValue&>(func).call();
        if (result.isError()) {
            throw IException(QString("Erron in evaluating constraint  (JS) for treelist (N=%1): %2").
                             arg(treelist->trees().size()).
                             arg(result.toString()));
        }
//        if (FMSTP::verbose())
//            qCDebug(abe) << "evaluate constraint (JS) for stand" << cell->id() << ":" << result.toString();
        // convert boolean result to 1 - 0
        return result.toBool();
//        if (result.isBool())
//            return result.toBool()==true?1.:0;
//        else
//            return result.toNumber();
    }

    }
    return true;


}

QString DynamicExpression::dump() const
{
    switch (filter_type){
    case ftInvalid: return "Invalid";
    case ftExpression: return expr->expression();
    case ftJavascript: return func.toString();
    default: return "invalid filter type!";
    }
}


/***************************************************************************/
/*************************  Constraints  ***********************************/
/***************************************************************************/

void Constraints::setup(QJSValue &js_value, DynamicExpression::EWrapperType wrap)
{
    mConstraints.clear();
    if ((js_value.isArray() || js_value.isObject()) && !js_value.isCallable()) {
        QJSValueIterator it(js_value);
        while (it.hasNext()) {
            it.next();
            if (it.name()==QStringLiteral("length"))
                continue;
            mConstraints.append(DynamicExpression());
            DynamicExpression &item = mConstraints.last();
            item.setup(it.value(), wrap);
        }
    } else {
        mConstraints.append(DynamicExpression());
        DynamicExpression &item = mConstraints.last();
        item.setup(js_value, wrap);

    }
}

double Constraints::evaluate(BiteCell *cell)
{
    if (mConstraints.isEmpty())
        return 1.; // no constraints to evaluate
    double p;
    double p_min = 1;
    for (int i=0;i<mConstraints.count();++i) {
        p = mConstraints.at(i).evaluate(cell);
        if (p == 0.) {
//            if (cell->trace())
//                qCDebug(abe) << cell->context() << "constraint" << mConstraints.at(i).dump() << "did not pass.";
            return 0.; // one constraint failed
        } else {
            // save the lowest value...
            p_min = std::min(p, p_min);
        }
    }
    return p_min; // all constraints passed, return the lowest returned value...
}

double Constraints::evaluate(ABE::FMTreeList *treelist)
{
    if (mConstraints.isEmpty())
        return 1.; // no constraints to evaluate
    double p;
    double p_min = 1;
    for (int i=0;i<mConstraints.count();++i) {
        p = mConstraints.at(i).evaluate(treelist);
        if (p == 0.) {
//            if (cell->trace())
//                qCDebug(abe) << cell->context() << "constraint" << mConstraints.at(i).dump() << "did not pass.";
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
        QJSValue val = BiteEngine::valueFromJs(js_value, event);
        if (val.isCallable()) {
            mEvents[event] = js_value; // save the event functions (and the name of the property that the function is assigned to)
        }
    }
}

QJSValue Events::run(const QString event, BiteCell *cell, QJSValueList *params)
{
    if (mEvents.contains(event)) {
//        if (cell)
//            FomeScript::setExecutionContext(cell);
        QMutexLocker lock(BiteEngine::instance()->serializeJS());

        QJSValue func = mEvents[event].property(event);
        QJSValue result;
        if (func.isCallable()) {
            DebugTimer t("BITE:JSEvents:run");

            if (params)
                result = func.callWithInstance(mInstance, *params);
            else
                result = func.callWithInstance(mInstance);
//            if (FMSTP::verbose() || (cell && cell->trace()))
//                qCDebug(abe) << (cell?cell->context():QString("<no stand>")) << "invoking javascript event" << event << " result: " << result.toString();
        }

        //qDebug() << "event called:" << event << "result:" << result.toString();
        if (result.isError()) {
            throw IException(QString("%3 Javascript error in event %1: %2").arg(event).arg(result.toString()).arg(cell?cell->index():-1));
        }
        return result;
    }
    return QJSValue();
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



} // end namespace
