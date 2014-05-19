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

namespace ABE {

ActSalvage::ActSalvage(FMSTP *parent): Activity(parent)
{
    mCondition = 0;
    mMaxPreponeActivity = 0;

    mBaseActivity.setEnabled(false); // avoid active scheduling...
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

    QString condition = FMSTP::valueFromJs(value, "disturbanceCondition").toString();
    if (!condition.isEmpty() && condition!="undefined") {
        mCondition = new Expression(condition);
    }
    mMaxPreponeActivity = FMSTP::valueFromJs(value, "maxPrepone", "0").toInt();
    mThresholdTotal = FMSTP::valueFromJs(value, "thresholdFullClearance", "0.95").toInt();
    mThresholdMinimal = FMSTP::valueFromJs(value, "thresholdSplitStand", "0.25").toInt();

}

bool ActSalvage::execute(FMStand *stand)
{
    // the salvaged timber is already accounted for - so nothing needs to be done here.
    // however, we check if there is a planned activity for the stand which could be executed sooner
    // than planned.
    bool preponed = const_cast<FMUnit*>(stand->unit())->scheduler()->forceHarvest(stand, mMaxPreponeActivity);
    if (stand->trace())
        qCDebug(abe) << "Salvage activity executed. Changed scheduled activites (preponed): " << preponed;

    const_cast<FMUnit*>(stand->unit())->scheduler()->addExtraHarvest(stand, stand->totalHarvest(), Scheduler::Salvage);
    stand->resetHarvestCounter(); // set back to zero...
    // check if we should re-assess the stand grid (after large disturbances)
    // as a preliminary check we only look closer, if we have more than 10m3/ha of damage.
    if (stand->disturbedTimber()/stand->area() > 10)
        checkStandAfterDisturbance();
    // the harvest happen(ed) anyways.
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

void ActSalvage::checkStandAfterDisturbance()
{
    //
    FMTreeList *trees = ForestManagementEngine::instance()->scriptBridge()->treesObj();
    //trees->runGrid();
    trees->prepareStandGrid(QStringLiteral("height"), QString());
    FloatGrid &grid = trees->standGrid();
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

    if (r_low < 0.1) {
        // no big damage: return and do nothing
        return;
    }
    if (r_low > 0.9) {
        // total disturbance: restart rotation...
        return;
    }
    // medium disturbance: check if need to split the stand area:
    Grid<int> my_map(grid.rectangle(), grid.cellsize());
    GridRunner<float> runner(grid, grid.metricRect());
    GridRunner<int> id_runner(my_map, grid.metricRect());
    float *neighbors[8];
    while (runner.next() && id_runner.next()) {
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
    }
}


} // namespace
