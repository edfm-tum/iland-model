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
}


} // namespace
