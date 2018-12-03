#include "bitecolonization.h"
#include "biteengine.h"
#include "bitecell.h"
namespace BITE {

BiteColonization::BiteColonization()
{

}

BiteColonization::BiteColonization(QJSValue obj): BiteItem(obj)
{

}

void BiteColonization::setup(BiteAgent *parent_agent)
{
    BiteItem::setup(parent_agent);

    try {

        checkProperties(mObj);
        QJSValue disp_filter = BiteEngine::valueFromJs(mObj, "dispersalFilter", "1", "required property.");
        mDispersalFilter.setup(disp_filter, DynamicExpression::CellWrap, parent_agent);


        QJSValue species_filter = BiteEngine::valueFromJs(mObj, "speciesFilter");
        if (!species_filter.isUndefined()) {
            qCDebug(biteSetup) << "species filter: " << species_filter.toString();
        }
        QJSValue cell_filter = BiteEngine::valueFromJs(mObj, "cellFilter");
        if (!cell_filter.isUndefined()) {
            qCDebug(biteSetup) << "cell filter: " << cell_filter.toString();
            mCellConstraints.setup(cell_filter, DynamicExpression::CellWrap, parent_agent);
        }
        QJSValue tree_filter = BiteEngine::valueFromJs(mObj, "treeFilter");
        if (!tree_filter.isUndefined()) {
            qCDebug(biteSetup) << "tree filter: " << tree_filter.toString();
            mTreeConstraints.setup(tree_filter, DynamicExpression::TreeWrap, parent_agent);
        }

        mInitialAgentBiomass = BiteEngine::valueFromJs(mObj, "initialAgentBiomass").toNumber();

        mThis = BiteEngine::instance()->scriptEngine()->newQObject(this);
        BiteAgent::setCPPOwnership(this);

        mEvents.setup(mObj, QStringList() << "onCalculate" << "onSetup", agent());

        QJSValueList eparam = QJSValueList() << thisJSObj();
        mEvents.run("onSetup", nullptr, &eparam);

    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of BiteColonization item '%1': %2").arg(name()).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }


}

void BiteColonization::runCell(BiteCell *cell, ABE::FMTreeList *treelist)
{
    // no colonization if agent is already living on the cell
    if (cell->isActive())
        return;

    if (mDispersalFilter.evaluateBool(cell)==false) {
        return;
    }

    if (agent()->verbose())
        qCDebug(bite) << "BiteCol:runCell:" << cell->index() << "of agent" << cell->agent()->name();

    ++agent()->stats().nColonizable;

    double result = mCellConstraints.evaluate(cell);
    if (result == 0.) {
        return; // no colonization
    }

    // now we need to load the trees
    cell->checkTreesLoaded(treelist);

    result = mTreeConstraints.evaluate(treelist);
    if (result == 0.) {
        return;
    }

    QJSValue event_res = mEvents.run("onCalculate", cell);
    if (event_res.isBool() && event_res.toBool()==false) {
        return; // event returned false
    }
    // successfully colonized
    cell->setActive(true);
    if (mInitialAgentBiomass>0.) {
        BiteWrapper bitewrap(agent()->wrapper(), cell);
        int iabm = bitewrap.variableIndex("agentBiomass");
        if (iabm<0) throw IException("variable 'agentBiomass' required.");
        bitewrap.setValue(iabm, mInitialAgentBiomass);
    }
    agent()->notifyItems(cell, BiteCell::CellColonized);
    ++agent()->stats().nNewlyColonized;

}

QStringList BiteColonization::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "speciesFilter" << "cellFilter" << "treeFilter" << "dispersalFilter";
    return l;

}


} // end namespace
