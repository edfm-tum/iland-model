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

bool BiteCellScript::hasValue(QString variable_name)
{
    Q_ASSERT(mAgent!=nullptr);
    return agent()->wrapper()->variableIndex(variable_name) >= 0;
}

double BiteCellScript::value(QString variable_name)
{
    Q_ASSERT(mAgent!=nullptr && mCell!=nullptr);
    BiteWrapper wrap(agent()->wrapper(), mCell);
    int var_idx = wrap.variableIndex(variable_name);
    if (var_idx < 0)
        throw IException(QString("Invalid variable '%1' for accessing cell variables (cell: %2, agent: %3).").arg(variable_name).arg(cell()->index()).arg(agent()->name()));
    return wrap.value(var_idx);

}

void BiteCellScript::setValue(QString var_name, double value)
{
    Q_ASSERT(mAgent!=nullptr && mCell!=nullptr);
    BiteWrapper wrap(agent()->wrapper(), mCell);
    int var_idx = wrap.variableIndex(var_name);
    if (var_idx < 0)
        throw IException(QString("Invalid variable '%1' for accessing cell variables (cell: %2, agent: %3).").arg(var_name).arg(cell()->index()).arg(agent()->name()));

    wrap.setValue(var_idx, value);

}

void BiteCellScript::reloadTrees()
{
    int n = mCell->loadTrees(trees());
    qCDebug(bite) << "reloaded trees for cell" << mCell->index() << ", N=" << n << " (treelist: " << trees();
}

// ***********************************************************
// *******************   DynamicExpression *******************
// ***********************************************************


DynamicExpression::DynamicExpression(const DynamicExpression &src)
{
    wrapper_type = src.wrapper_type;
    if (src.expr)
        expr = new Expression(src.expr->expression());
    else
        expr = nullptr;

    qCDebug(bite) << "dynamicexpression: copy ctor...";
}

DynamicExpression::~DynamicExpression()
{
    if (expr)
        delete expr;
    if (mTree)
        delete mTree;
}

void DynamicExpression::setup(const QJSValue &js_value, EWrapperType type, BiteAgent *agent)
{
    mAgent = agent;
    filter_type = ftInvalid;
    wrapper_type = type;
    if (expr) delete expr;
    expr=nullptr;
    if (js_value.isString() && js_value.toString().isEmpty())
        return; // keep invalid

    mScriptCell = BiteEngine::instance()->scriptEngine()->newQObject(&mCell);
    BiteAgent::setCPPOwnership(&mCell);
    mTree = new ScriptTree;
    mTreeValue = BiteEngine::instance()->scriptEngine()->newQObject(mTree);


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
    if (js_value.isNumber()) {
        filter_type = ftConstant;
        mConstValue = js_value.toNumber();
        return;
    }
    throw IException(QString("Invalid input to a dynamic expression: '%1' is not a Javascript function, nor a expression or a number.").arg(js_value.toString()));
}

double DynamicExpression::evaluate(BiteCell *cell) const
{
    switch (filter_type) {
    case ftInvalid: return true; // message?
    case ftConstant: return mConstValue;
    case ftExpression: {
        BiteWrapper bitewrap(mAgent->wrapper(), cell);
        double result;
        try {
            result = expr->execute(nullptr, &bitewrap); // using execute, we're in strict mode, i.e. wrong variables are reported.
        } catch (IException &e) {
            // throw a nicely formatted error message
            e.add(QString("in filter (expr: '%2') for cell %1.").
                  arg(cell->index()).
                  arg(expr->expression()) );
            throw;
        }

        //            if (FMSTP::verbose())
        //                qCDebug(abe) << cell->context() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << cell->id() << ":" << result;
        return result;

    }
    case ftJavascript: {

        QMutexLocker lock(BiteEngine::instance()->serializeJS());

        // call javascript function with parameter
        const_cast<BiteCellScript&>(mCell).setCell(cell);
        const_cast<BiteCellScript&>(mCell).setAgent(mAgent);

        QJSValue result = const_cast<QJSValue&>(func).call(QJSValueList() << mScriptCell);
        if (result.isError() || result.isUndefined()) {
            throw IException(QString("Error in evaluating constraint (or no return value) (JS) for cell %1: %2").
                             arg(cell->index()).
                             arg(result.toString()));
        }
        if (mAgent->verbose())
            qCDebug(bite) << "evaluate dynamic expression (JS) for cell" << cell->index() << ":" << result.toString();

        // convert boolean result to 1 - 0
        if (result.isBool())
            return result.toBool()==true?1.:0;
        else
            return result.toNumber();
    }

    }
    return true;
}

double DynamicExpression::evaluate(Tree *tree) const
{
    switch (filter_type) {
    case ftInvalid: return true; // message?
    case ftConstant: return mConstValue;
    case ftExpression: {
        TreeWrapper treewrap(tree);
        double result;
        try {
            result = expr->execute(nullptr, &treewrap); // using execute, we're in strict mode, i.e. wrong variables are reported.
            //result = expr->calculate(wrapper);
        } catch (IException &e) {
            // throw a nicely formatted error message
            e.add(QString("in filter (expr: '%2') for tree (N=%1).").
                  arg(tree->id()).
                  arg(expr->expression()) );
            throw;
        }

        //            if (FMSTP::verbose())
        //                qCDebug(abe) << cell->context() << "evaluate constraint (expr:" << expr->expression() << ") for stand" << cell->id() << ":" << result;
        return result;

    }
    case ftJavascript: {
        // call javascript function
        // provide the execution context

        QMutexLocker lock(BiteEngine::instance()->serializeJS());
        mTree->setTree(tree);

        QJSValue result = const_cast<QJSValue&>(func).call(QJSValueList() << mScriptCell);
        if (result.isError() || result.isUndefined()) {
            throw IException(QString("Error in evaluating constraint  (or undefined return value) (JS) for tree (N=%1): %2").
                             arg(tree->id()).
                             arg(result.toString()));
        }
        //        if (FMSTP::verbose())
        //            qCDebug(abe) << "evaluate constraint (JS) for stand" << cell->id() << ":" << result.toString();
        // convert boolean result to 1 - 0

        if (result.isBool())
            return result.toBool()==true?1.:0;
        else
            return result.toNumber();
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
    case ftConstant: return QString::number(mConstValue);
    //default: return "invalid filter type!";
    }
    return QLatin1Literal("unknown");
}


/***************************************************************************/
/*************************  Constraints  ***********************************/
/***************************************************************************/

void Constraints::setup(QJSValue &js_value, DynamicExpression::EWrapperType wrap, BiteAgent *agent)
{
    mAgent = agent;
    mConstraints.clear();
    if ((js_value.isArray() || js_value.isObject()) && !js_value.isCallable()) {
        QJSValueIterator it(js_value);
        while (it.hasNext()) {
            it.next();
            if (it.name()==QStringLiteral("length"))
                continue;
            mConstraints.append(DynamicExpression());
            DynamicExpression &item = mConstraints.last();
            item.setup(it.value(), wrap, agent);
        }
    } else {
        mConstraints.append(DynamicExpression());
        DynamicExpression &item = mConstraints.last();
        item.setup(js_value, wrap, agent);

    }
}

double Constraints::evaluate(BiteCell *cell)
{
    if (mConstraints.isEmpty())
        return 1.; // no constraints to evaluate
    double p;
    double p_min = 1;
    for (int i=0;i<mConstraints.count();++i) {
        p = mConstraints.at(i).evaluateBool(cell);
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


    bool found = false;
    for (auto t : treelist->trees()) {
        Tree *tree = t.first;

        for (int i=0;i<mConstraints.count();++i) {
            if (mConstraints.at(i).evaluateBool(tree)) {
                found = true;
                return 1.; // done! at least one tree passes one constraint
            }
        }
    }

    // no tree meets any of the constraints
    return 0.;
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

void Events::setup(QJSValue &js_value, QStringList event_names, BiteAgent *agent)
{
    mInstance = js_value; // save the object that contains the events
    mAgent = agent;
    foreach (QString event, event_names) {
        QJSValue val = BiteEngine::valueFromJs(js_value, event);
        if (val.isCallable()) {
            mEvents[event] = js_value; // save the event functions (and the name of the property that the function is assigned to)
        }
    }
    mScriptCell = BiteEngine::instance()->scriptEngine()->newQObject(&mCell);
    BiteAgent::setCPPOwnership(&mCell);
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
            else {
                if (cell) {
                    // default parameter: the cell
                    mCell.setCell(cell);
                    mCell.setAgent(mAgent);
                    result = func.callWithInstance(mInstance, QJSValueList() << mScriptCell);
                } else {
                    result = func.callWithInstance(mInstance);
                }
            }

            if (result.isError()) {
                throw IException(QString("%3 Javascript error in event %1: %2").arg(event).arg(result.toString()).arg(cell?cell->index():-1));
            }
            return result;
        }
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
