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
        mKillAllHostTrees = BiteEngine::valueFromJs(mObj, "killAllHostTrees", "false").toBool();
        mSimulate = BiteEngine::valueFromJs(mObj, "simulate", "false").toBool();

        filter = BiteEngine::valueFromJs(mObj, "impactFilter");
        mImpactFilter.setup(filter, DynamicExpression::CellWrap, parent_agent);

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
    if (mKillAllHostTrees) {
        int killed;
        if (mSimulate)
            killed = treelist->count(); // simulation mode
        else
            killed = treelist->kill(QString()); // real impact

        if (verbose())
            qCDebug(bite) << "Impact: killed all host trees (n=" << killed << ")";

        agent()->notifyItems(cell, BiteCell::CellImpacted);
        mEvents.run("onAfterImpact", cell);
        return;
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
    l << "impactFilter" << "hostTrees" << "killAllHostTrees";
    return l;

}




} // end namespace
