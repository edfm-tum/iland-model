#include "abe_global.h"
#include "actthinning.h"

#include "fmstand.h"
#include "fmtreelist.h"
#include "fmstp.h"

#include <QJSValueIterator>
namespace ABE {

ActThinning::ActThinning(FMSTP *parent): Activity(parent)
{
    mBaseActivity.setIsScheduled(true); // use the scheduler
    mBaseActivity.setDoSimulate(true); // simulate per default
    mThinningType = Invalid;
}

QString ActThinning::type() const
{
    QString th;
    switch (mThinningType) {
    case Invalid: th="Invalid"; break;
    case FromBelow: th="from below"; break;
    case FromAbove: th="from above"; break;
    case Custom: th = "custom"; break;
    case Selection: th = "selection"; break;
    }

    return QString("thinning (%1)").arg(th);
}

void ActThinning::setup(QJSValue value)
{
    Activity::setup(value); // setup base events
    mThinningType = Invalid;
    QString th_type = FMSTP::valueFromJs(value, "thinning").toString();
    if (th_type=="fromBelow") mThinningType=FromBelow;
    else if (th_type=="fromAbove") mThinningType=FromAbove;
    else if (th_type=="custom") mThinningType=Custom;
    else if (th_type=="selection") mThinningType=Selection;
    else
        throw IException(QString("Setup of thinning: invalid thinning type: %1").arg(th_type));

    switch (mThinningType) {
    case Custom: setupCustom(value); break;
    default: throw IException("No setup defined for thinning type");
    }

}

bool ActThinning::evaluate(FMStand *stand)
{
    switch (mThinningType) {
    case Custom: return evaluateCustom(stand);
    default: throw IException("ActThinning::evaluate: not available for thinning type");
    }
    return false;
}

bool ActThinning::execute(FMStand *stand)
{
    if (stand->trace()) qCDebug(abe) << stand->context() << "execute  activity" << name() << ":" << type();
    if (events().hasEvent(QStringLiteral("onExecute"))) {
        // switch off simulation mode
        stand->currentFlags().setDoSimulate(false);
        // execute this event
        bool result =  Activity::execute(stand);
        stand->currentFlags().setDoSimulate(true);
        return result;
    } else {
        // default behavior: process all marked trees (harvest / cut)
        if (stand->trace()) qCDebug(abe) << stand->context() << "activity" << name() << "remove all marked trees.";
        FMTreeList trees(stand);
        trees.removeMarkedTrees();
        return true;
    }

}

// setup of the "custom" thinning operation
void ActThinning::setupCustom(QJSValue value)
{
    mCustom.usePercentiles = FMSTP::boolValueFromJs(value, "percentile", true);
    mCustom.removal = FMSTP::boolValueFromJs(value, "removal", true);
    mCustom.relative = FMSTP::boolValueFromJs(value, "relative", true);
    mCustom.remainingStems = FMSTP::valueFromJs(value, "remainingStems", "0").toInt();
    mCustom.minDbh = FMSTP::valueFromJs(value, "minDbh", "0").toNumber();
    mCustom.targetVariable = FMSTP::valueFromJs(value, "targetVariable", "stems").toString();
    if (mCustom.targetVariable != "stems" &&
            mCustom.targetVariable != "basalArea" &&
            mCustom.targetVariable != "volume")
        throw IException(QString("setup of custom Activity: invalid targetVariable: %1").arg(mCustom.targetVariable));

    QJSValue values = FMSTP::valueFromJs(value, "values",  "", "setup custom acitvity");
    if (!values.isArray())
        throw IException("setup of custom activity: the 'values' is not an array.");
    mCustom.classValues.clear();
    mCustom.classPercentiles.clear();
    QJSValueIterator it(values);
    while (it.hasNext()) {
        it.next();
        if (it.name()==QStringLiteral("length"))
            continue;
        mCustom.classValues.push_back(it.value().toNumber());
    }
    if (mCustom.classValues.size()==0)
        throw IException("setup of custom thinnings: 'values' has no elements.");
    double f = 100. / mCustom.classValues.size();
    double p = 0.;
    for (int i=0;i<mCustom.classValues.size();++i, p+=f)
        mCustom.classPercentiles.push_back(qRound(p));
    mCustom.classPercentiles.push_back(100);
}

bool ActThinning::evaluateCustom(FMStand *stand)
{
    FMTreeList trees(stand);
    if (mCustom.minDbh>0.)
        trees.load(QString("dbh>%1").arg(mCustom.minDbh));
    else
        trees.loadAll();
    if (mCustom.remainingStems>0 && mCustom.remainingStems<trees.trees().size())
        return false;

    if (trees.trees().size()==0)
        return false;

    // sort always by target variable (if it is stems, then simply by dbh)
    bool target_dbh = mCustom.targetVariable=="stems";
    if (target_dbh)
        trees.sort("dbh");
    else
        trees.sort(mCustom.targetVariable);

    // count trees and values (e.g. volume) in the defined classes
    QVector<double> values = mCustom.classValues; // make a copy
    QVector<double> tree_counts = mCustom.classValues; // make a copy

    QVector<QPair<Tree*, double> >::const_iterator it;
    for (int i=0;i<values.count();++i) {
        values[i]=0.;
        tree_counts[i]=0.;
    }
    int class_index = 0;
    int n=0;
    double tree_count=trees.trees().count();
    double total_value = 0.;
    for (it = trees.trees().constBegin(); it!=trees.trees().constEnd(); ++it, ++n) {
        if (n/tree_count*100. > mCustom.classPercentiles[class_index+1])
            ++class_index;
        tree_counts[class_index]++;
        values[class_index]+= it->second;
        total_value += it->second;
    }
    // the simplest case
    if (mCustom.relative) {
        for (int i=0;i<values.count();++i) {
            if (mCustom.removal)
                // remove xx% of the list
                values[i] = tree_counts[i] * mCustom.classValues[i]/100.;
            else
                // xx% should remain, i.e. remove 100-xx%
                values[i] = (tree_count -  tree_counts[i]) * mCustom.classValues[i]/100.;
        }
    }
    int total_remove = 0;
    for (int i=0;i<values.count();++i) {
        total_remove += values[i];
    }
    if (total_remove==0)
        return false;

    // reduce removals if the total removal would result in less trees then
    // defined with "remainingStems". All classes are scaled with the same factor.
    if (tree_count - total_remove < mCustom.remainingStems*stand->area()) {
        double factor = mCustom.remainingStems*stand->area() / total_remove;
        for (int i=0;i<values.count();++i)
            values[i] *= factor;
    }
    // in each class of values we have now the number of trees that should be removed in the list.
    for (int i=0;i<values.count();++i) {
        if (values[i]>0.)
            trees.remove_percentiles(mCustom.classPercentiles[i], mCustom.classPercentiles[i+1], values[i], true);
    }

    return true;


}


} // namespace
