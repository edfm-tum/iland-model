#include "bite_global.h"
#include "biteagent.h"
#include "biteengine.h"

#include "model.h"
#include "fmtreelist.h"

#include <QQmlEngine>

namespace BITE {

QHash<QThread*, ABE::FMTreeList* > BiteAgent::mTreeLists;

BiteAgent::BiteAgent(QObject *parent): QObject(parent)
{

}

BiteAgent::BiteAgent(QJSValue obj): QObject(nullptr)
{
    setup(obj);
}

void BiteAgent::setup(QJSValue obj)
{
    qCDebug(biteSetup) << "*** BITE: Setup of a new agent ***";
    try {
        mName = BiteEngine::valueFromJs(obj, "name", "",  "'name' is a required property!").toString();
        mDesc = BiteEngine::valueFromJs(obj, "description", "",  "'description' is a required property!").toString();
        mCellSize = BiteEngine::valueFromJs(obj, "cellSize", "",  "'cellSize' is a required property!").toInt();
        if ( !(mCellSize == 10 || mCellSize==20 || mCellSize==50 || mCellSize==100) )
            throw IException("Invalid value for cell size! Allowed sized are 10,20,50, and 100m.");

        createBaseGrid();

        // extract properties from the input object
        if (obj.isObject()) {
            QJSValueIterator it(obj);
            while (it.hasNext()) {
                it.next();
                qCDebug(biteSetup) << it.name() << " = " << it.value().toString();
                if (it.value().isQObject()) {
                    // check if it is a valid item and add
                    QObject *qobj = it.value().toQObject();
                    BiteItem *bitem = qobject_cast<BiteItem*>(qobj);
                    if (bitem) {
                        // setup the item
                        bitem->setName(it.name());
                        bitem->setup(this);
                        // the object is managed from now on by C++
                        QQmlEngine::setObjectOwnership(bitem, QQmlEngine::CppOwnership);
                        mItems.push_back(bitem);
                        qCDebug(biteSetup) << "added item #" << mItems.count() << ", " << bitem->name();
                    }
                }
            }

        }
    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of Bite agent '%1': %2").arg(mName).arg(e.message());
        qCDebug(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }
    qCDebug(biteSetup) << "*** Setup of a agent complete ***";

}

//void _run_cell(BiteCell &cell) {
//    cell.agent()->runCell(cell);
//}

void BiteAgent::run()
{
    // main function
    // step 1: run all phase-level items (e.g. dispersal)
    for (auto item : mItems) {
        if (item->runCells() == false)
            item->run();
    }

    // step 2: run cell-by-cell functions parallel
    GlobalSettings::instance()->model()->threadExec().run<BiteCell>( &BiteAgent::runCell, mCells);

    qCDebug(bite) << "Agent" << name() << "finished";
}

BiteItem *BiteAgent::item(QString name)
{
    for (int i=0;i<mItems.count();++i)
        if (mItems[i]->name() == name)
            return mItems[i];
    return nullptr;
}

void BiteAgent::runCell(BiteCell &cell)
{
    // cells have to be active!
    if (!cell.isActive())
        return;

    // main function: loop over all items and call runCell
    ABE::FMTreeList *tree_list = threadTreeList();

    // fetch trees for the cell
    cell.loadTrees(tree_list);
    for (const auto &p : cell.agent()->mItems)
        if (p->runCells()) {
            p->runCell(&cell, tree_list);
    }
}

static QMutex _thread_treelist;
ABE::FMTreeList *BiteAgent::threadTreeList()
{
    if (BiteAgent::mTreeLists.contains(QThread::currentThread()))
        return mTreeLists[QThread::currentThread()];
    QMutexLocker lock(&_thread_treelist);
    mTreeLists[QThread::currentThread()] = new ABE::FMTreeList;
    return mTreeLists[QThread::currentThread()];

}

void BiteAgent::createBaseGrid()
{
    // setup the internal grid
    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    mGrid.clear();
    mGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                cellSize());
    mGrid.initialize(nullptr);
    mCells.clear();

    int index = 0;
    for (auto *bc = mGrid.begin(); bc!=mGrid.end(); ++bc, ++index) {
        QPointF pos = mGrid.cellCenterPoint(index);
        if (hg->constValueAt(pos).isValid()) {
            *bc = (BiteCell*)(mCells.size()); // for now, just store the position
            mCells.push_back(BiteCell());
            mCells.back().setup(index, pos, this);
        }
    }

    //
    for (auto *bc = mGrid.begin(); bc!=mGrid.end(); ++bc) {
        if (bc) {
            *bc = &mCells[ (size_t)(*bc)  ]; // store the real pointer to the mCells array
        }
    }

    qCDebug(biteSetup) << "Agent: " << name() << ": setup of base grid (cellSize:" << cellSize() << "), " << mCells.size() << "cells created";

}


} // end namespace
