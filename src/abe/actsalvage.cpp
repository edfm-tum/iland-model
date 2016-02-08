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
#include "abe_global.h"
#include "actsalvage.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "scheduler.h"
#include "forestmanagementengine.h"
#include "fmtreelist.h"

#include "tree.h"
#include "expression.h"
#include "expressionwrapper.h"
#include "mapgrid.h"
#include "helper.h"

namespace ABE {

/** @class ActSalvage
    @ingroup abe
    The ActSalvage class handles salvage logging after disturbances.

  */

ActSalvage::ActSalvage(FMSTP *parent): Activity(parent)
{
    mCondition = 0;
    mMaxPreponeActivity = 0;

    mBaseActivity.setIsSalvage(true);
    mBaseActivity.setIsRepeating(true);
    mBaseActivity.setExecuteImmediate(true);

}

ActSalvage::~ActSalvage()
{
    if (mCondition)
        delete mCondition;
}

void ActSalvage::setup(QJSValue value)
{
    Activity::setup(value); // setup base events
    events().setup(value, QStringList() << "onBarkBeetleAttack");

    QString condition = FMSTP::valueFromJs(value, "disturbanceCondition").toString();
    if (!condition.isEmpty() && condition!="undefined") {
        mCondition = new Expression(condition);
    }
    mMaxPreponeActivity = FMSTP::valueFromJs(value, "maxPrepone", "0").toInt();
    mThresholdSplit = FMSTP::valueFromJs(value, "thresholdSplitStand", "0.1").toNumber();
    mThresholdClear = FMSTP::valueFromJs(value, "thresholdClearStand", "0.9").toNumber();
    mThresholdMinimal = FMSTP::valueFromJs(value, "thresholdIgnoreDamage", "5").toNumber();
    mDebugSplit = FMSTP::boolValueFromJs(value, "debugSplit", false);

}

bool ActSalvage::execute(FMStand *stand)
{


    if (stand->property("_run_salvage").toBool()) {
        // 2nd phase: do the after disturbance cleanup of a stand.
        bool simu = stand->currentFlags().isDoSimulate();
        stand->currentFlags().setDoSimulate(false);
        // execute the "onExecute" event
        bool result =  Activity::execute(stand);
        stand->currentFlags().setDoSimulate(simu);
        stand->setProperty("_run_salvage", false);
        return result;
    }

    // the salvaged timber is already accounted for - so nothing needs to be done here.
    // however, we check if there is a planned activity for the stand which could be executed sooner
    // than planned.
    bool preponed = const_cast<FMUnit*>(stand->unit())->scheduler()->forceHarvest(stand, mMaxPreponeActivity);
    if (stand->trace())
        qCDebug(abe) << "Salvage activity executed. Changed scheduled activites (preponed): " << preponed;

    const_cast<FMUnit*>(stand->unit())->scheduler()->addExtraHarvest(stand, stand->totalHarvest(), Scheduler::Salvage);
    // check if we should re-assess the stand grid (after large disturbances)
    // as a preliminary check we only look closer, if we have more than  x m3/ha of damage.
    if (stand->disturbedTimber()/stand->area() > mThresholdMinimal)
        checkStandAfterDisturbance(stand);

    // the harvest happen(ed) anyways.
    stand->resetHarvestCounter(); // set back to zero...
    return true;
}

QStringList ActSalvage::info()
{
    QStringList lines = Activity::info();
    lines << QString("condition: %1").arg(mCondition?mCondition->expression():"-");
    lines << QString("maxPrepone: %1").arg(mMaxPreponeActivity);
    return lines;
}

bool ActSalvage::evaluateRemove(Tree *tree) const
{
    if (!mCondition)
        return true; // default: remove all trees
    TreeWrapper tw(tree);
    bool result = mCondition->execute(0, &tw);
    return result;
}

bool ActSalvage::barkbeetleAttack(FMStand *stand, double generations, int infested_px_ha)
{

    //QJSValue params;
    QJSValueList params=QJSValueList() << QJSValue(generations) << QJSValue(infested_px_ha);

    QJSValue result = events().run(QStringLiteral("onBarkBeetleAttack"), stand, &params);
    if (!result.isBool())
        qCDebug(abe) << "Salvage-Activity:onBarkBeetleAttack: expecting a boolean return";
    return result.toBool();
}

void ActSalvage::checkStandAfterDisturbance(FMStand *stand)
{
    //
    FMTreeList *trees = ForestManagementEngine::instance()->scriptBridge()->treesObj();
    //trees->runGrid();
    trees->prepareStandGrid(QStringLiteral("height"), QString());

    FloatGrid &grid = trees->standGrid();
    static int no_split = 0;
    if (mDebugSplit)
        trees->exportStandGrid(QString("temp/height_%1.txt").arg(++no_split));

    float h_max = grid.max();

    double r_low;
    if (h_max==0.f) {
        // total disturbance...
        r_low = 1.;
    } else {
        // check coverage of disturbed area.
        int h_lower = 0, h_higher=0;
        for (float *p=grid.begin(); p!=grid.end(); ++p)
            if (*p>=0.f) {
                if (*p < h_max*0.33)
                    ++h_lower;
                else
                    ++h_higher;
            }
        if (h_lower==0 && h_higher==0)
            return;
        r_low = h_lower / double(h_lower+h_higher);
    }

    if (r_low < mThresholdSplit) {
        // no big damage: return and do nothing
        return;
    }
    if (r_low > mThresholdClear) {
        // total disturbance: restart rotation...
        stand->setProperty("_run_salvage", true);
        stand->reset(stand->stp());
        return;
    }
    // medium disturbance: check if need to split the stand area:
    Grid<int> my_map(grid.cellsize(), grid.sizeX(), grid.sizeY());
    GridRunner<float> runner(&grid);
    GridRunner<int> id_runner(&my_map);
    float *neighbors[8];
    int n_empty=0;
    while (runner.next() && id_runner.next()) {
        if (*runner.current()==-1.) {
            *id_runner.current() = -1;
            continue;
        }
        runner.neighbors8(neighbors);
        double empty = 0.;
        int valid = 0;
        for (int i=0;i<8;++i) {
            if (neighbors[i] && *neighbors[i]<h_max*0.33)
                empty++;
            if (neighbors[i])
                valid++;
        }
        if (valid)
            empty /= double(valid);
        // empty cells are marked with 0; areas covered by forest set to stand_id; -1: out-of-stand areas
        // if a cell is empty, some neighbors (i.e. >50%) need to be empty too;
        // if a cell is *not* empty, it has to be surrounded by a larger fraction of empty points (75%)
        if ( (*runner.current()<h_max*0.33 && empty>0.5)
             || (empty>=0.75) ) {
            *id_runner.current() = 0;
            n_empty++;
        } else {
            *id_runner.current() = stand->id();
        }
    }
    if (mDebugSplit)
       Helper::saveToTextFile(GlobalSettings::instance()->path(QString("temp/split_before_%1.txt").arg(no_split)), gridToESRIRaster(my_map) );


    // now flood-fill 0ed areas....
    // if the "new" areas are too small (<0.25ha), then nothing happens.
    QVector<int> new_stands;
    int fill_color = -1;
    id_runner.reset();
    while (id_runner.next()) {
        if (*id_runner.current()==0)
            if (floodFillHelper(my_map, id_runner.currentIndex(), --fill_color)>25) {
                new_stands.push_back(fill_color);
            }
    }
    if (mDebugSplit)
       Helper::saveToTextFile(GlobalSettings::instance()->path(QString("temp/split_stands_%1.txt").arg(no_split)), gridToESRIRaster(my_map) );

    for (int i=0;i<new_stands.size(); ++i) {
        // ok: we have new stands. Now do the actual splitting
        FMStand *new_stand = ForestManagementEngine::instance()->splitExistingStand(stand);
        // copy back to the stand grid
        GridRunner<int> sgrid(ForestManagementEngine::instance()->standGrid()->grid(), grid.metricRect());
        id_runner.reset();
        while (sgrid.next() && id_runner.next()) {
            if (*id_runner.current() == new_stands[i])
                *sgrid.current() = new_stand->id();
        }

        // the new stand  is prepared.
        // at the end of this years execution, the stand will be re-evaluated.
        new_stand->reset(stand->stp());

    }


}

// quick and dirty implementation of the flood fill algroithm.
// based on: http://en.wikipedia.org/wiki/Flood_fill
// returns the number of pixels colored
int ActSalvage::floodFillHelper(Grid<int> &grid, QPoint start, int color)
{
    QQueue<QPoint> pqueue;
    pqueue.enqueue(start);
    int found = 0;
    while (!pqueue.isEmpty()) {
        QPoint p = pqueue.dequeue();
        if (!grid.isIndexValid(p))
            continue;
        if (grid.valueAtIndex(p)==0) {
            grid.valueAtIndex(p) = color;
            pqueue.enqueue(p+QPoint(-1,0));
            pqueue.enqueue(p+QPoint(1,0));
            pqueue.enqueue(p+QPoint(0,-1));
            pqueue.enqueue(p+QPoint(0,1));
            ++found;
        }
    }
    return found;
}


} // namespace
