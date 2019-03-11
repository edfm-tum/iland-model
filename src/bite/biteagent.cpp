/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#include "bite_global.h"
#include "biteagent.h"
#include "biteengine.h"

#include "model.h"
#include "fmtreelist.h"
#include "fmsaplinglist.h"
#include "scriptgrid.h"
#include "bitelifecycle.h"

#include <QQmlEngine>

namespace BITE {

QHash<QThread*, ABE::FMTreeList* > BiteAgent::mTreeLists;
QHash<QThread*, ABE::FMSaplingList* > BiteAgent::mSaplingLists;

BiteAgent::BiteAgent(QObject *parent): QObject(parent)
{

}

BiteAgent::BiteAgent(QJSValue obj): QObject(nullptr)
{
    mVerbose = false;
    mLC = nullptr;
    setup(obj);
}

BiteAgent::~BiteAgent()
{
    // delete all the items
    for (auto *p : mItems)
        delete p;


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

        // setup climate variables
        QJSValue clim_vars = BiteEngine::valueFromJs(obj, "climateVariables", "");
        if (clim_vars.isArray())
            mClimateProvider.setup(clim_vars, mWrapperCore);

        // setup the base grid for the agent
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
                        setCPPOwnership(bitem);

                        mItems.push_back(bitem);
                        qCDebug(biteSetup) << "added item #" << mItems.count() << ", " << bitem->name();

                        if (qobject_cast<BiteLifeCycle*>(bitem) )
                            mLC=qobject_cast<BiteLifeCycle*>(bitem);
                    }
                }
            }

        }

        mThis = BiteEngine::instance()->scriptEngine()->newQObject(this);
        BiteAgent::setCPPOwnership(this);

        // additional set up routines after all items are there
        for (int i=0;i<mItems.size();++i)
            mItems[i]->afterSetup();

        mEvents.setup(obj, QStringList() << "onSetup" << "onYearBegin" << "onYearEnd", this);
        QJSValueList eparam = QJSValueList() << mThis;
        mEvents.run("onSetup", nullptr, &eparam);


        if (mLC==nullptr)
            throw IException("A 'BiteLifeCycle' object is mandatory!");
        BiteEngine::instance()->addAgent(this);
    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of Bite agent '%1': %2").arg(mName).arg(e.message());
        qCDebug(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }
    qCDebug(biteSetup) << "*** Setup of a agent complete ***";

}

void BiteAgent::setCPPOwnership(QObject *obj)
{
    QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
}

void BiteAgent::notifyItems(BiteCell *cell, BiteCell::ENotification what)
{
    cell->notify(what);
    for (auto item : mItems) {
        item->notify(cell, what);
    }

}

QStringList BiteAgent::variables()
{
 return wrapper()->getVariablesList();
}

//void _run_cell(BiteCell &cell) {
//    cell.agent()->runCell(cell);
//}

void BiteAgent::run()
{
    stats().clear(); // reset stats

    // main function
    QJSValueList eparam = QJSValueList() << mThis;
    mEvents.run("onYearBegin", nullptr, &eparam);

    for (auto item : mItems) {
        item->beforeRun();
    }

    // step 1: run all phase-level items (e.g. dispersal)
    for (auto item : mItems) {
        if (item->runCells() == false)
            item->run();
    }

    // step 2: run cell-by-cell functions parallel
    try {
        GlobalSettings::instance()->model()->threadExec().run<BiteCell>( &BiteAgent::runCell, mCells);
    } catch (const IException &e) {
        qCWarning(bite) << "An error occured while running the agent" << name() << ":" << e.what();
        throw IException(QString("Bite: Error while running agent: %1: %2").arg(name()).arg(e.what()));
    }

    mEvents.run("onYearEnd", nullptr, &eparam);

    qCDebug(bite) << "Agent" << name() << "finished";
    qCDebug(bite) << "NSpread:" << stats().nDispersal << "NColonizable:" << stats().nColonizable << "NColonized:" << stats().nNewlyColonized;
}

void BiteAgent::run(BiteCellScript *cell)
{
    BiteCell *c = cell->cell();
    qCDebug(bite) << "execute run for cell" << c->info();
    runCell(*c);
}

BiteItem *BiteAgent::item(QString name)
{
    for (int i=0;i<mItems.count();++i)
        if (mItems[i]->name() == name)
            return mItems[i];
    return nullptr;
}

QString BiteAgent::info()
{
    QString msg = QString("Agent: %1\nCell-size: %2\nDescription: %3\n").arg(name()).arg(cellSize()).arg(mDesc);
    msg += "\n=========================\n";
    for (int i=0;i<mItems.count();++i)
        msg += "Item: " + mItems[i]->name() +
                "\n=========================\n" + mItems[i]->info() + "\n";

    msg += QString("Variables: %1").arg(wrapper()->getVariablesList().join(","));
    return msg;
}

double BiteAgent::evaluate(BiteCellScript *cell, QString expr)
{
    BiteWrapper bw(&mWrapperCore, cell->cell());
    Expression expression(expr, &bw);
    double value = expression.execute();
    return value;
}

void BiteAgent::addVariable(ScriptGrid *grid, QString var_name)
{
    wrapper()->registerGridVar(grid->grid(), var_name);
    grid->setName(var_name);
    grid->setOwnership(false); // the grid is now managed by BITE (and freed from the wrapper)
    qCDebug(biteSetup) << "added a grid (" << grid->name() << ") to the agent" << name();
}

void BiteAgent::updateDrawGrid(QString expression)
{
    BiteWrapper wrap(wrapper());
    Expression expr(expression, &wrap);
    BiteCell **cell = mGrid.begin();
    for (double *p = mBaseDrawGrid.begin(); p!=mBaseDrawGrid.end(); ++p, ++cell)
        if (*cell) {
            wrap.setCell(*cell);
            *p = expr.execute();
        }
}

void BiteAgent::updateDrawGrid(QJSValue func)
{
    if (!func.isCallable())
        throw IException("BiteAgent::updateDrawGrid - no function provided!");
    BiteCellScript bcs;
    QJSValue js_scriptcell = BiteEngine::instance()->scriptEngine()->newQObject(&bcs);

    BiteCell **cell = mGrid.begin();
    for (double *p = mBaseDrawGrid.begin(); p!=mBaseDrawGrid.end(); ++p, ++cell)
        if (*cell) {
            bcs.setCell(*cell);
            QJSValue result = func.call(QJSValueList() << js_scriptcell);
            if (!result.isNumber())
                throw IException("BiteAgent::updateDrawCell: return of Javascript function not numeric! Result:" + result.toString());
            *p = result.toNumber();
        }
}

void BiteAgent::saveGrid(QString expression, QString file_name)
{
    updateDrawGrid(expression);
    drawGrid()->save(file_name);
    qCDebug(bite) << "Saved grid of agent" << name() << ":" << expression << "to" << file_name;
}

void BiteAgent::runCell(BiteCell &cell)
{

    // main function: loop over all items and call runCell
    ABE::FMTreeList *tree_list = threadTreeList();
    ABE::FMSaplingList *sap_list = threadSaplingList();

    try {

    // fetch trees for the cell
    cell.setTreesLoaded(false);
    cell.setSaplingsLoaded(false);
    for (const auto &p : cell.agent()->mItems)
        if (p->runCells()) {
            p->runCell(&cell, tree_list, sap_list);
    }
    cell.finalize(); // some cleanup and stats
    } catch (const IException &e) {
        // report error
        BiteEngine::instance()->error( e.message() );
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

ABE::FMSaplingList *BiteAgent::threadSaplingList()
{
    if (BiteAgent::mSaplingLists.contains(QThread::currentThread()))
        return mSaplingLists[QThread::currentThread()];
    QMutexLocker lock(&_thread_treelist);
    mSaplingLists[QThread::currentThread()] = new ABE::FMSaplingList;
    return mSaplingLists[QThread::currentThread()];

}

BiteCellScript *BiteAgent::cell(int x, int y)
{
    if (!isCellValid(x,y) || mGrid.valueAtIndex(x,y)==nullptr)
        throw IException(QString("BiteAgent:cell: invalid position %1/%2 (agent %3)").arg(x).arg(y).arg(name()));
    BiteCell *c=mGrid.valueAtIndex(x,y);
    BiteCellScript *bcs = new BiteCellScript();
    bcs->setCell(c);
    bcs->setAgent(this);
    return bcs;
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
    BiteCell *null_cell = nullptr;
    for (auto *bc = mGrid.begin(); bc!=mGrid.end(); ++bc, ++index) {
        QPointF pos = mGrid.cellCenterPoint(index);
        if (hg->constValueAt(pos).isValid()) {
            mCells.push_back(BiteCell());
            mCells.back().setup(index, pos, this);
            BiteCell *bcp = null_cell + static_cast<size_t>(mCells.size()); //  just to avoid compiler warnings...
            *bc = bcp;
        }
    }

    //
    for (auto *bc = mGrid.begin(); bc!=mGrid.end(); ++bc) {
        if (*bc) {
            *bc = &mCells[ (static_cast<int>((*bc) - null_cell)) - 1 ]; // store the real pointer to the mCells array
        }
    }

    qCDebug(biteSetup) << "Agent: " << name() << ": setup of base grid (cellSize:" << cellSize() << "), " << mCells.size() << "cells created";

    mBaseDrawGrid.setup(mGrid.metricRect(), mGrid.cellsize());
    mBaseDrawGrid.initialize(0.);

    mDrawGrid = new ScriptGrid(&mBaseDrawGrid);
    mDrawGrid->setOwnership(false); // scriptgrid should not delete the grid


}


} // end namespace
