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

        bool has_cc_tree = mObj.hasOwnProperty("hostBiomassTree");
        bool has_cc_cell = mObj.hasOwnProperty("hostBiomassCell");

        if ( (has_cc_cell && has_cc_tree) || (!has_cc_cell && !has_cc_tree))
            throw IException("specify either 'hostBiomassTree' OR 'hostBiomassCell'! ");
        if (has_cc_cell) {
            QJSValue calc_cc = BiteEngine::valueFromJs(mObj, "hostBiomassCell");
            mCalcHBMCell.setup(calc_cc, DynamicExpression::CellWrap, parent_agent);
        }
        if (has_cc_tree) {
            QJSValue calc_cc = BiteEngine::valueFromJs(mObj, "hostBiomassTree");
            mCalcHBMTree.setup(calc_cc, DynamicExpression::TreeWrap, parent_agent);
        }

        QJSValue grfun = BiteEngine::valueFromJs(mObj, "growthFunction");
        if (!grfun.isUndefined()) {
            mGrowthFunction.addVar("M");
            mGrowthFunction.addVar("K");
            mGrowthFunction.addVar("r");
            mGrowthFunction.addVar("t");
            mGrowthFunction.setExpression(grfun.toString());
            mGrowthRateFunction.setExpression(BiteEngine::valueFromJs(mObj, "growthRateFunction", "", "'growthRateFunction' is mandatory if 'growthFunction' is used!").toString());
            mGrowthIterations = BiteEngine::valueFromJs(mObj, "growthIterations", "1", "'growthIterations' is mandatory if 'growthFunction' is used!").toInt();
            if (mGrowthIterations<1)
                throw IException("Invalid value: growthIterations < 1!");
            mGrowthConsumption = BiteEngine::valueFromJs(mObj, "consumption", "1", "'consumption' is mandatory if 'growthFunction' is used!").toNumber();
            if (mGrowthConsumption==0.)
                throw IException("0 is not a valid 'consumption'.");
        }
        QJSValue mort = BiteEngine::valueFromJs(mObj, "mortality", "", "'mortality' is a required property");
        mMortality.setup(mort, DynamicExpression::CellWrap, parent_agent);
        mVerbose = BiteEngine::valueFromJs(mObj, "verbose", "false").toBool();

        // setup the variables / grids

        mHostBiomass.setup(agent()->grid().metricRect(), agent()->grid().cellsize());
        mHostBiomass.initialize(0.);
        mAgentBiomass.setup(agent()->grid().metricRect(), agent()->grid().cellsize());
        mAgentBiomass.initialize(0.);
        mImpact.setup(agent()->grid().metricRect(), agent()->grid().cellsize());
        mImpact.initialize(0.);

        agent()->wrapper()->registerGridVar(&mHostBiomass, "hostBiomass");
        agent()->wrapper()->registerGridVar(&mAgentBiomass, "agentBiomass");
        agent()->wrapper()->registerGridVar(&mImpact, "agentImpact");

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
        mHostBiomass[cell->index()] = 0.;
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

    // (2) calculate host biomass
    if (mCalcHBMCell.isValid()) {
        double host_biomass= mCalcHBMCell.evaluate(cell);
        if (isnan(host_biomass))
            throw IException("BiteBiomass: host biomass is NaN! Expr:" + mCalcHBMCell.dump());
        mHostBiomass[cell->index()] = host_biomass;
        if (agent()->verbose())
            qCDebug(bite) << "host biomass (cell):" << host_biomass;

    }
    if (mCalcHBMTree.isValid()) {
        double host_biomass = 0.;
        for (int i=0;i<treelist->count(); ++i)
            host_biomass  += mCalcHBMTree.evaluate(treelist->trees()[i].first);

        if (isnan(host_biomass ))
            throw IException("BiteBiomass: host biomass is NaN! Expr:" + mCalcHBMTree.dump());

        mHostBiomass[cell->index()] = host_biomass ;
        if (agent()->verbose())
            qCDebug(bite) << "host biomass (" << treelist->count() << "trees):" << host_biomass ;
    }

    // (3) calculate biomass
    if (!mGrowthFunction.isEmpty()) {
        // growth calculation using expressions (logistic type)
        calculateLogisticGrowth(cell);
    } else {
        double biomass_before = mAgentBiomass[cell->index()];
        if (mEvents.hasEvent("onCalculate")) {
            double bm = mEvents.run("onCalculate", cell).toFloat();
            if (isnan(bm))
                throw IException("BiteBiomass: agent biomass (return of onCalculate) is NaN!");

            mAgentBiomass[cell->index()] = bm;
        } else {
            qCDebug(bite) << "no action... TODO...";
        }
        double biomass_after = mAgentBiomass[cell->index()];
        if (agent()->verbose())
            qCDebug(bite) << "biomass before:" << biomass_before << ", new biomass:" << biomass_after;
    }

    // (4) Mortality
    double p_mort = mMortality.evaluate(cell);
    if (drandom() < p_mort) {
        cell->die();
        mEvents.run("onMortality", cell);
        if (agent()->verbose())
            qCDebug(bite) << "cell died due to mortality: index" << cell->index();
    }

    agent()->stats().agentBiomass += mAgentBiomass[cell->index()];

    mEvents.run("onExit", cell);
}

void BiteBiomass::beforeRun()
{
    // clear impact
    mImpact.initialize(0.);
}

QStringList BiteBiomass::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "hostTrees" << "hostBiomassCell" << "hostBiomassTree" << "mortality"
      << "growthFunction" << "growthRateFunction" << "growthIterations" << "verbose" << "consumption";
    return l;

}

void BiteBiomass::calculateLogisticGrowth(BiteCell *cell)
{
    // Variables: M: current agent biomass
    // r: growth rate for the year
    // t: running time variable

    // Step 1: calculate growth rate
    if (mVerbose)
        qCDebug(bite) << "** calcuate biomass growth for" << agent()->name() << ", cell: " << cell->index();
    BiteWrapper bitewrap(agent()->wrapper(), cell);
    double growth_rate = mGrowthRateFunction.execute(nullptr, &bitewrap);
    if (mVerbose)
        qCDebug(bite) << "growth-rate:" << growth_rate;
    double agent_biomass = mAgentBiomass[cell->index()]; // initial biomass
    double host_biomass = mHostBiomass[cell->index()]; // host biomass

    if (host_biomass==0.) {
        if (mVerbose)
            qCDebug(bite) << "host biomass is 0. Setting agentBiomass to 0.";
        mAgentBiomass[cell->index()] = 0.;
        return;
    }

    // when Expressions are executed concurrently it is important  to use local variables
    double local_var_space[4];
    double *biomassptr = &local_var_space[0]; // M: current agent biomass
    double *agentcc = &local_var_space[1]; // K: agent max biomass
    double *rptr = &local_var_space[2]; // r: growth rate
    double *tptr = &local_var_space[3]; // t: time step

    *tptr = 1. / static_cast<double>(mGrowthIterations); // each step is 1/N_steps
    *agentcc = host_biomass / mGrowthConsumption; // initial value for 'K'

    for (int i=0;i<mGrowthIterations;++i) {
        if (mVerbose)
            qCDebug(bite) << "Iteration" << i << "/" << mGrowthIterations << ": host biomass:" << host_biomass << "agent biomass (before):" << agent_biomass;

        *biomassptr = agent_biomass; // 'M'
        *rptr = growth_rate; // 'r'

        agent_biomass = mGrowthFunction.execute(local_var_space, &bitewrap);
        if (agent_biomass == 0.) {
            if (mVerbose)
                qCDebug(bite) << "updated agent biomass is 0. Stopping.";
            break;
        }

        // consumption during time step: agent(kg) * consumption(kgHost/kgAgent*yr) * time (e.g. 1/10)
        host_biomass = host_biomass - mGrowthConsumption * agent_biomass * *tptr;
        if (host_biomass<0.) {
            host_biomass = 0.; // nothing is left
            agent_biomass = 0.; // no host, no agent
            if (mVerbose)
                qCDebug(bite) << "updated host biomass is 0. Stopping.";
            break;


        }
        *agentcc = host_biomass / mGrowthConsumption; // update max agent biomass
    }
    if (mVerbose || agent()->verbose()) {
        qCDebug(bite) << "Updated agentBiomass for " <<  agent()->name() << ", cell: " << cell->index() << ":" << agent_biomass;
    }
    // write back
    mAgentBiomass[cell->index()] = agent_biomass;
    mImpact[cell->index()] = mHostBiomass[cell->index()] - host_biomass; // the impact

}



} // end namespace