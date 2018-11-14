#include "bitebiomass.h"
#include "biteengine.h"
#include "fmtreelist.h"

namespace BITE {

BiteBiomass::BiteBiomass(QJSValue obj): BiteItem(obj)
{

}

void BiteBiomass::setup(BiteAgent *parent_agent)
{
    BiteItem::setup(parent_agent);
    try {

        checkProperties(mObj);

        mHostTreeFilter = BiteEngine::valueFromJs(mObj, "hostTrees").toString();

        bool has_cc_tree = mObj.hasOwnProperty("carryingCapacityTree");
        bool has_cc_cell = mObj.hasOwnProperty("carryingCapacityCell");

        if ( (has_cc_cell && has_cc_tree) || (!has_cc_cell && !has_cc_tree))
            throw IException("specify either 'carryingCapacityTree' OR 'carryingCapacityCell'! ");
        if (has_cc_cell) {
            QJSValue calc_cc = BiteEngine::valueFromJs(mObj, "carryingCapacityCell");
            mCalcCCCell.setup(calc_cc, DynamicExpression::CellWrap, parent_agent);
        }
        if (has_cc_tree) {
            QJSValue calc_cc = BiteEngine::valueFromJs(mObj, "carryingCapacityTree");
            mCalcCCTree.setup(calc_cc, DynamicExpression::TreeWrap, parent_agent);
        }

        QJSValue mort = BiteEngine::valueFromJs(mObj, "mortality", "", "'mortality' is a required property");
        mMortality.setup(mort, DynamicExpression::CellWrap, parent_agent);

        // setup the variables / grids

        mCarryingCapacity.setup(agent()->grid().metricRect(), agent()->grid().cellsize());
        mCarryingCapacity.initialize(0.);
        mAgentBiomass.setup(agent()->grid().metricRect(), agent()->grid().cellsize());
        mAgentBiomass.initialize(0.);

        agent()->wrapper()->registerGridVar(&mCarryingCapacity, "carryingCapacity");
        agent()->wrapper()->registerGridVar(&mAgentBiomass, "agentBiomass");

        mThis = BiteEngine::instance()->scriptEngine()->newQObject(this);
        BiteAgent::setCPPOwnership(this);

        mEvents.setup(mObj, QStringList() << "onCalculate" << "onEnter" << "onExit" << "onSetup" << "onMortality", agent());

        QJSValueList eparam = QJSValueList() << thisJSObj();
        mEvents.run("onSetup", nullptr, &eparam);




    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of BiteBiomass item '%1': %2").arg(name()).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }
}

QString BiteBiomass::info()
{
    QString res = QString("Type: BiteBiomass\nDesc: %1").arg(description());
    return res;

}

void BiteBiomass::notify(BiteCell *cell, BiteCell::ENotification what)
{
    switch (what) {
    case BiteCell::CellDied:
        // clear biomass
        mCarryingCapacity[cell->index()] = 0.;
        mAgentBiomass[cell->index()] = 0.;
        break;

    default: break; // ignore other notifications
    }
}



void BiteBiomass::runCell(BiteCell *cell, ABE::FMTreeList *treelist)
{
    if (!cell->isActive())
        return;

    // (1) apply the host tree filter on the tree list
    cell->checkTreesLoaded(treelist); // load trees (if this has not happened before)

    int before = treelist->count();
    int after = treelist->filter(mHostTreeFilter);
    if (verbose()) {
        qCDebug(bite) << "Biomass: filter trees with" << mHostTreeFilter << "N before:" << before << ", after: " << after;
    }

    // (2) calculate carrying capacity
    if (mCalcCCCell.isValid()) {
        double carrying_cap = mCalcCCCell.evaluate(cell);
        if (isnan(carrying_cap))
            throw IException("BiteBiomass: carrying capacity is NaN! Expr:" + mCalcCCCell.dump());
        mCarryingCapacity[cell->index()] = carrying_cap;
        if (agent()->verbose())
            qCDebug(bite) << "carrying capacity (cell):" << carrying_cap;

    }
    if (mCalcCCTree.isValid()) {
        double carrying_cap = 0.;
        for (int i=0;i<treelist->count(); ++i)
            carrying_cap += mCalcCCTree.evaluate(treelist->trees()[i].first);

        if (isnan(carrying_cap))
            throw IException("BiteBiomass: carrying capacity is NaN! Expr:" + mCalcCCTree.dump());

        mCarryingCapacity[cell->index()] = carrying_cap;
        if (agent()->verbose())
            qCDebug(bite) << "carrying capacity (" << treelist->count() << "trees):" << carrying_cap;
    }

    // (3) calculate biomass
    double biomass_before = mAgentBiomass[cell->index()];
    if (mEvents.hasEvent("onCalculate")) {
        double bm = mEvents.run("onCalculate", cell).toNumber();
        if (isnan(bm))
            throw IException("BiteBiomass: biomass (return of onCalculate) is NaN!");

        mAgentBiomass[cell->index()] = bm;
    } else {
        qCDebug(bite) << "no action... TODO...";
    }
    double biomass_after = mAgentBiomass[cell->index()];
    if (agent()->verbose())
        qCDebug(bite) << "biomass before:" << biomass_before << ", new biomass:" << biomass_after;


    // (4) Mortality
    double p_mort = mMortality.evaluate(cell);
    if (drandom() < p_mort) {
        cell->die();
        mEvents.run("onMortality", cell);
        if (agent()->verbose())
            qCDebug(bite) << "cell died due to mortality: index" << cell->index();
    }

    mEvents.run("onExit", cell);
}

QStringList BiteBiomass::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "hostTrees" << "carryingCapacityCell" << "carryingCapacityTree" << "mortality" << "growthFunction";
    return l;

}



} // end namespace
