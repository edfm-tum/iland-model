#include "abe_global.h"
#include "actthinning.h"

#include "fmstand.h"
#include "fmtreelist.h"
#include "fmstp.h"

#include "tree.h"

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
    bool return_value = true;
    switch (mThinningType) {
    case Custom:
        for (int i=0;i<mCustomThinnings.count();++i)
            return_value = return_value && evaluateCustom(stand, mCustomThinnings[i]);
        return return_value; // false if one fails

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

void ActThinning::setupCustom(QJSValue value)
{
    mCustomThinnings.clear();
    if (value.hasProperty("thinnings") && value.property("thinnings").isArray()) {
        QJSValueIterator it(value.property("thinnings"));
        while (it.hasNext()) {
            it.next();
            if (it.name()==QStringLiteral("length"))
                continue;
            mCustomThinnings.push_back(SCustomThinning());
            setupSingleCustom(it.value(), mCustomThinnings.back());
        }
    } else {
        mCustomThinnings.push_back(SCustomThinning());
        setupSingleCustom(value, mCustomThinnings.back());
    }
}

// setup of the "custom" thinning operation
void ActThinning::setupSingleCustom(QJSValue value, SCustomThinning &custom)
{
    custom.usePercentiles = FMSTP::boolValueFromJs(value, "percentile", true);
    custom.removal = FMSTP::boolValueFromJs(value, "removal", true);
    custom.relative = FMSTP::boolValueFromJs(value, "relative", true);
    custom.remainingStems = FMSTP::valueFromJs(value, "remainingStems", "0").toInt();
    custom.minDbh = FMSTP::valueFromJs(value, "minDbh", "0").toNumber();
    QJSValue filter = FMSTP::valueFromJs(value, "filter", "");
    if (filter.isString())
        custom.filter = filter.toString();
    else
        custom.filter = QString();
    custom.targetVariable = FMSTP::valueFromJs(value, "targetVariable", "stems").toString();
    if (custom.targetVariable != "stems" &&
            custom.targetVariable != "basalArea" &&
            custom.targetVariable != "volume")
        throw IException(QString("setup of custom Activity: invalid targetVariable: %1").arg(custom.targetVariable));

    custom.targetRelative = FMSTP::boolValueFromJs(value, "targetRelative", true);
    custom.targetValue = FMSTP::valueFromJs(value, "targetValue", "30").toNumber();
    if (custom.targetRelative && (custom.targetValue>100. || custom.targetValue<0.))
        throw IException(QString("setup of custom Activity: invalid relative targetValue (0-100): %1").arg(custom.targetValue));

    QJSValue values = FMSTP::valueFromJs(value, "classes",  "", "setup custom acitvity");
    if (!values.isArray())
        throw IException("setup of custom activity: the 'classes' is not an array.");
    custom.classValues.clear();
    custom.classPercentiles.clear();
    QJSValueIterator it(values);
    while (it.hasNext()) {
        it.next();
        if (it.name()==QStringLiteral("length"))
            continue;
        custom.classValues.push_back(it.value().toNumber());
    }
    if (custom.classValues.size()==0)
        throw IException("setup of custom thinnings: 'classes' has no elements.");

    // check if sum is 100 for relative classes
    if (custom.relative) {
        double sum=0.;
        for (int i=0;i<custom.classValues.size();++i)
            sum+=custom.classValues[i];
        if (fabs(sum-100.)>0.000001)
            throw IException("setup of custom thinnings: 'classes' do not add up to 100 (relative=true).");
    }

    double f = 100. / custom.classValues.size();
    double p = 0.;
    for (int i=0;i<custom.classValues.size();++i, p+=f)
        custom.classPercentiles.push_back(qRound(p));
    custom.classPercentiles.push_back(100);
}

bool ActThinning::evaluateCustom(FMStand *stand, SCustomThinning &custom)
{
    FMTreeList trees(stand);
    QString filter = custom.filter;
    if (custom.minDbh>0.) {
        if (!filter.isEmpty())
            filter += " and ";
        filter += QString("dbh>%1").arg(custom.minDbh);
    }

    if (!filter.isEmpty())
        trees.load(filter);
    else
        trees.loadAll();

    if (custom.remainingStems>0 && custom.remainingStems<trees.trees().size())
        return false;

    if (trees.trees().size()==0)
        return false;

    // remove harvest flags.
    clearTreeMarks(&trees);

    // sort always by target variable (if it is stems, then simply by dbh)
    bool target_dbh = custom.targetVariable=="stems";
    if (target_dbh)
        trees.sort("dbh");
    else
        trees.sort(custom.targetVariable);

    // count trees and values (e.g. volume) in the defined classes
    QVector<double> values = custom.classValues; // make a copy
    QVector<double> tree_counts = custom.classValues; // make a copy
    QVector<int> percentiles = custom.classPercentiles; // make a copy

    QVector<QPair<Tree*, double> >::const_iterator it;
    for (int i=0;i<values.count();++i) {
        tree_counts[i]=0.;
    }
    int class_index = 0;
    int n=0;

    percentiles.first()=0;
    double tree_count=trees.trees().count();
    double total_value = 0.;
    for (it = trees.trees().constBegin(); it!=trees.trees().constEnd(); ++it, ++n) {
        if (n/tree_count*100. > custom.classPercentiles[class_index+1]) {
            ++class_index;
            percentiles[class_index] = n; // then n'th tree
        }
        tree_counts[class_index]++;
        total_value += target_dbh?1.:it->second; // e.g., sum of volume in the class, or simply count
    }
    percentiles.last()=n+1;

    double target_value=0.;
    if (custom.targetRelative)
        target_value = custom.targetValue * total_value / 100.;
    else
        target_value = custom.targetValue * stand->area();

    if (!custom.relative) {
        // class values are given in absolute terms, e.g. 40m3/ha.
        // this needs to be translated to relative classes.
        // if the demand in a class cannot be met (e.g. planned removal of 40m3/ha, but there are only 20m3/ha in the class),
        // then the "miss" is distributed to the other classes (via the scaling below).
        for (int i=0;i<values.size();++i) {
            if (values[i]>0){
                if (values[i]<=custom.classValues[i]*stand->area()) {
                    values[i] = 1.;
                } else {
                    values[i] = custom.classValues[i]*stand->area() / values[i];
                }
            }
        }
        // scale to 100
        double sum=0.;
        for (int i=0;i<values.size();++i)
            sum+=values[i];
        if (sum>0.){
            for (int i=0;i<values.size();++i)
                values[i] *= 100./sum;
        }
    }

    // *****************************************************************
    // ***************    Main loop
    // *****************************************************************
    bool finished = false;
    int cls;
    double p;
    int removed_trees = 0;
    double removed_value = 0.;
    do {
        // look up a random number: it decides in which class to select a tree:
        p = nrandom(0,100);
        for (cls=0;cls<values.size();++cls) {
            if (p < values[cls])
                break;
            p-=values[cls];
        }
        // select a tree:
        int tree_idx = selectRandomTree(&trees, percentiles[cls], percentiles[cls+1]-1);
        if (tree_idx>=0) {
            // stop harvesting, when the target size is reached: if the current tree would surpass the limit,
            // a random number decides whether the tree should be included or not.
            double tree_value = target_dbh?1.:trees.trees()[tree_idx].second;
            if (custom.targetValue>0.) {
                if (removed_value + tree_value > target_value)
                    if (drandom()>0.5)
                        break;
            }
            trees.remove_single_tree(tree_idx, true);
            removed_trees++;
            removed_value += tree_value;
        }
        // stop harvesting, when the minimum remaining number of stems is reached
        if (trees.trees().size()-removed_trees <= custom.remainingStems*stand->area())
            finished = true;

        if (custom.targetValue>0. && removed_value > target_value)
            finished = true;

    } while (!finished);

    return true;

}

int ActThinning::selectRandomTree(FMTreeList *list, const int pct_min, const int pct_max)
{
    // pct_min, pct_max: the indices of the first and last tree in the list to be looked for, including pct_max
    // seek a tree in the class 'cls' (which has not already been removed);
    int idx;
    // search randomly for a couple of times
    for (int i=0;i<5;i++) {
        idx = irandom(pct_min, pct_max);
        Tree *tree = list->trees()[idx].first;
        if (!tree->isDead() && !tree->isMarkedForHarvest() && !tree->isMarkedForCut())
            return idx;
    }
    // not found, now walk in a random direction...
    int direction = 1;
    if (drandom()>0.5) direction=-1;
    // start in one direction from the last selected random position
    int ridx=idx;
    while (ridx>=pct_min && ridx<=pct_max) {
        Tree *tree = list->trees()[ridx].first;
        if (!tree->isDead() && !tree->isMarkedForHarvest() && !tree->isMarkedForCut())
            return ridx;

        ridx+=direction;
    }
    // now look in the other direction
    direction = -direction;
    ridx = idx;
    while (ridx>=pct_min && ridx<=pct_max) {
        Tree *tree = list->trees()[ridx].first;
        if (!tree->isDead() && !tree->isMarkedForHarvest() && !tree->isMarkedForCut())
            return ridx;

        ridx+=direction;
    }

    // no tree found in the entire range
    return -1;


}

void ActThinning::clearTreeMarks(FMTreeList *list)
{
    QVector<QPair<Tree*, double> >::const_iterator it;
    for (it=list->trees().constBegin(); it!=list->trees().constEnd(); ++it) {
        Tree *tree = it->first;
        if (tree->isMarkedForHarvest())
            tree->markForHarvest(false);
        if (tree->isMarkedForCut())
            tree->markForCut(false);

    }
}


} // namespace
