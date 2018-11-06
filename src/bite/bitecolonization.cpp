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
        QJSValue species_filter = BiteEngine::valueFromJs(mObj, "speciesFilter");
        if (!species_filter.isUndefined()) {
            qCDebug(biteSetup) << "species filter: " << species_filter.toString();
        }
        QJSValue cell_filter = BiteEngine::valueFromJs(mObj, "cellFilter");
        if (!cell_filter.isUndefined()) {
            qCDebug(biteSetup) << "cell filter: " << cell_filter.toString();
            mCellConstraints.setup(cell_filter, DynamicExpression::CellWrap);
        }
        QJSValue tree_filter = BiteEngine::valueFromJs(mObj, "treeFilter");
        if (!tree_filter.isUndefined()) {
            qCDebug(biteSetup) << "tree filter: " << tree_filter.toString();
            mTreeConstraints.setup(tree_filter, DynamicExpression::TreeWrap);
        }

        mEvents.setup(mObj, QStringList() << "onCalculate");

    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of BiteColonization item '%1': %2").arg(name()).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }


}

void BiteColonization::runCell(BiteCell *cell, ABE::FMTreeList *treelist)
{
    if (!cell->isActive())
        return;
    qCDebug(bite) << "BiteCol:runCell:" << cell->index() << "of agent" << cell->agent()->name();
    ++agent()->stats().nColonizable;

    double result = mCellConstraints.evaluate(cell);
    if (result == 0.) {
        cell->setActive(false);
        return; // no colonization
    }

    result = mTreeConstraints.evaluate(treelist);
    if (result == 0.) {
        cell->setActive(false);
        return;
    }

    QJSValue event_res = mEvents.run("onCalculate", cell);
    if (event_res.isBool() && event_res.toBool()==false) {
        cell->setActive(false);
        return; // event returned false
    }
    // successfull colonized
    ++agent()->stats().nColonized;

}

QStringList BiteColonization::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "speciesFilter" << "cellFilter" << "treeFilter";
    return l;

}


} // end namespace
