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
        mSimulate = BiteEngine::valueFromJs(mObj, "simulate", "false").toBool();

        filter = BiteEngine::valueFromJs(mObj, "impactFilter");
        mImpactFilter.setup(filter, DynamicExpression::CellWrap, parent_agent);


        mImpactMode = Invalid;
        QString impact_mode = BiteEngine::valueFromJs(mObj, "impactMode").toString();
        if (impact_mode=="foliage") mImpactMode=Foliage;
        if (impact_mode=="killAll") mImpactMode=KillAll;
        if (mImpactMode == Invalid)
            throw IException("Invalid impact mode: " + impact_mode);

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
    if (mImpactMode == KillAll) {
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

    if (mImpactMode == Foliage) {
        BiteWrapper bitewrap(agent()->wrapper(), cell);

        int ivar = bitewrap.variableIndex("agentImpact");
        if (ivar < 0)
            throw IException("variable 'agentImpact' not available in BiteImpact");
        double to_remove = bitewrap.value(ivar);
        qCDebug(bite) << "Impact: remove" << to_remove << "of foliage from cell" << cell->index() << " (not impl!)";
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
    l << "impactFilter" << "hostTrees" << "killAllHostTrees" << "impactMode" << "isSpreading";
    return l;

}




} // end namespace
