#include "biteimpact.h"
#include "biteengine.h"
#include "fmtreelist.h"

namespace BITE {

BiteImpact::BiteImpact(QJSValue obj): BiteItem(obj)
{

}

void BiteImpact::setup(BiteAgent *parent_agent)
{
    BiteItem::setup(parent_agent);
    try {

        checkProperties(mObj);

        QJSValue filter =  BiteEngine::valueFromJs(mObj, "hostTrees", "");
        if (!filter.isUndefined())
            mHostTreeFilter = filter.toString();
        mSimulate = BiteEngine::valueFromJs(mObj, "simulate").toBool(); // default false

        filter = BiteEngine::valueFromJs(mObj, "impactFilter");
        mImpactFilter.setup(filter, DynamicExpression::CellWrap, parent_agent);


        QString impact_target = BiteEngine::valueFromJs(mObj, "impactTarget").toString();
        int idx = (QStringList() << "killAll" << "foliage").indexOf(impact_target);
        if (idx<0)
            throw IException("Invalid impactTarget: " + impact_target);
        mImpactTarget = static_cast<ImpactTarget>(idx);


        QString impact_mode = BiteEngine::valueFromJs(mObj, "impactMode").toString();
        idx = (QStringList() << "relative" << "removeAll" << "removePart").indexOf(impact_mode);
        if (idx<0)
            throw IException("Invalid impactMode: " + impact_mode);
        mImpactMode = static_cast<ImpactMode>(idx);

        QString impact_order = BiteEngine::valueFromJs(mObj, "impactOrder").toString();
        if (impact_order != "undefined")
            mImportOrder = impact_order;

        mVerbose = BiteEngine::valueFromJs(mObj, "verbose").toBool();

        mEvents.setup(mObj, QStringList() << "onImpact" << "onAfterImpact" << "onExit" , agent());



    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of BiteImpact item '%1': %2").arg(name()).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }

}

QString BiteImpact::info()
{
    QString res = QString("Type: BiteImpact\nDesc: %1").arg(description());
    return res;

}

void BiteImpact::afterSetup()
{
    BiteWrapper bitewrap(agent()->wrapper());

    iAgentImpact = bitewrap.variableIndex("agentImpact");
    if (iAgentImpact < 0)
        throw IException("variable 'agentImpact' not available in BiteImpact");

}

void BiteImpact::runCell(BiteCell *cell, ABE::FMTreeList *treelist)
{
    if (!cell->isActive())
        return;

    bool filter = mImpactFilter.evaluateBool(cell);
    if (verbose())
        qCDebug(bite) << "Impact: result of impactFilter:" << filter;
    if (!filter)
        return;

    // filter host trees
    cell->checkTreesLoaded(treelist); // load trees (if this has not happened before)
    if (!mHostTreeFilter.isEmpty()) {
        int before = treelist->count();
        int after = treelist->filter(mHostTreeFilter);
        if (verbose())
            qCDebug(bite) << "Impact: filter trees with" << mHostTreeFilter << "N before:" << before << ", after: " << after;

    }

    // now either kill all or run a function
    if (mImpactTarget == KillAll) {
        int killed;
        double killed_m3 = 0.;
        for (int i=0;i<treelist->count();++i)
            killed_m3+=treelist->trees()[i].first->volume();
        if (mSimulate)
            killed = treelist->count(); // simulation mode
        else
            killed = treelist->kill(QString()); // real impact

        if (verbose())
            qCDebug(bite) << "Impact: killed all host trees (n=" << killed << ")";

        agent()->notifyItems(cell, BiteCell::CellImpacted);
        agent()->stats().treesKilled += killed;
        agent()->stats().m3Killed += killed_m3;

        mEvents.run("onAfterImpact", cell);
        return;
    }

    if (mImpactTarget == Foliage) {
        BiteWrapper bitewrap(agent()->wrapper(), cell);

        double to_remove = bitewrap.value(iAgentImpact);

        double realized_impact = doImpact(to_remove, cell, treelist);
        agent()->stats().totalImpact += realized_impact;

    }

    int killed = mEvents.run("onImpact", cell).toInt();
    if (verbose())
        qCDebug(bite) << "Impact: called 'onImpact', #trees killed (=return value): " << killed;

    if (killed>0) {
        agent()->notifyItems(cell, BiteCell::CellImpacted);
    }


}

QStringList BiteImpact::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "impactFilter" << "hostTrees" << "impactTarget"  << "impactOrder" <<  "impactMode" << "verbose" << "simulate";
    return l;

}

double BiteImpact::doImpact(double to_remove, BiteCell *cell, ABE::FMTreeList *treelist)
{
    double remove_per_tree = to_remove;
    if (treelist->count()==0)
        return 0.;

    if (mImpactMode == Relative) {
        // calculate amount of removal for each tree
        double total_biomass = 0.;
        for (int i=0;i<treelist->count();++i) {
            switch (mImpactTarget) {
            case Foliage: total_biomass+=treelist->trees()[i].first->biomassFoliage(); break;
            default: break;
            }
        }
        if (total_biomass==0.) {
            qCWarning(bite) << "BiteImpact: no actual biomass to remove. Cell:" << cell->info();
            return 0.;
        }
        remove_per_tree = to_remove / total_biomass;
    }
    if (mImpactMode == RemovePart)
        remove_per_tree = to_remove / treelist->count();

    // now apply sorting if present
    if (!mImportOrder.isEmpty())
        treelist->sort(mImportOrder);

    // apply the impact
    double applied_impact = 0.;
    double frac_fol=0., frac_stem=0., frac_branch=0.;
    int n_affected=0;
    for (int i=0;i<treelist->count();++i, ++n_affected) {
        Tree *tree = treelist->trees()[i].first;
        // currently *only* foliage
        double comp_rem;
        double *pool=nullptr;
        switch(mImpactTarget) {
        case Foliage: comp_rem = tree->biomassFoliage(); pool = &frac_fol; break;
        default: comp_rem = 0.;
        }
        if (comp_rem==0. || pool==nullptr) {
            qCDebug(bite) << "Impact: tree:" << tree->dump() << "has no biomass to affect! cell:" << cell->info();
            continue;
        }

        if (mImpactMode == Relative) {
            applied_impact += comp_rem * remove_per_tree; // remove_per_tree: is a fraction
            *pool = remove_per_tree;
        } else {
            // maximum removal: the total biomass of the tree OR the remaining impact
            double rem_tree = qMin(qMin(comp_rem, remove_per_tree), to_remove-applied_impact); // remove_per_tree is biomass (kg)
            applied_impact += rem_tree;
            *pool = rem_tree / comp_rem;
        }

        if (!mSimulate)
            tree->removeBiomassOfTree(frac_fol, frac_stem, frac_branch);

        if (mVerbose)
            qCDebug(bite) << "Impact:"<<  cell->info() << ":" << tree->dump() << ": removed fol%:" << frac_fol*100. << "stem%:" << frac_stem*100. << "branch%:" << frac_branch*100.;
        if (applied_impact>=to_remove)
            break;
    }
    if (agent()->verbose() || mVerbose)
        qCDebug(bite) << "Impact: removed" << applied_impact << "from" << n_affected+1 <<"trees (Ntrees: " << treelist->count() << ") (target:" << to_remove << "). cell" << cell->info();

    return applied_impact;
}




} // end namespace
