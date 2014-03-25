#include "global.h"
#include "amie_global.h"
#include "fomescript.h"
#include "forestmanagementengine.h"
#include "fmstp.h"
#include "agenttype.h"
#include "fmtreelist.h"

namespace AMIE {



FomeScript::FomeScript(QObject *parent) :
    QObject(parent)
{
    mStandObj = 0;
    mSiteObj = 0;
    mSimulationObj = 0;
    mTrees = 0;
}

FomeScript::~FomeScript()
{
    if (mStandObj) {
        delete mStandObj;
        delete mSiteObj;
        delete mSimulationObj;
        delete mTrees;
    }
}

void FomeScript::setupScriptEnvironment()
{
    // create javascript objects in the script engine
    // these objects can be accessed from Javascript code representing forest management activities
    // or agents.

    // stand variables
    mStandObj = new StandObj;
    QJSValue stand_value = ForestManagementEngine::scriptEngine()->newQObject(mStandObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("stand", stand_value);

    // site variables
    mSiteObj = new SiteObj;
    QJSValue site_value = ForestManagementEngine::scriptEngine()->newQObject(mSiteObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("site", site_value);

    // general simulation variables (mainly scenariolevel)
    mSimulationObj = new SimulationObj;
    QJSValue simulation_value = ForestManagementEngine::scriptEngine()->newQObject(mSimulationObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("simulation", simulation_value);

    // general simulation variables (mainly scenariolevel)
    mTrees = new FMTreeList;
    QJSValue treelist_value = ForestManagementEngine::scriptEngine()->newQObject(mTrees);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("trees", treelist_value);

    // the script object itself
    QJSValue script_value = ForestManagementEngine::scriptEngine()->newQObject(this);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("fmengine", script_value);

}

void FomeScript::setExecutionContext(const FMStand *stand)
{
    FomeScript *bridge =ForestManagementEngine::instance()->scriptBridge();
    bridge->mStandObj->setStand(stand);
    bridge->mTrees->setStandId(stand->id());

    bridge->mSiteObj->setStand(stand);

    // simulation stuff....

}


bool FomeScript::addManagement(QJSValue program, QString name)
{
    try {
        FMSTP *stp = new FMSTP();
        stp->setup(program, name);
        ForestManagementEngine::instance()->addSTP(stp);
        return true;
    } catch (const IException &e) {
        qCWarning(abe) << e.message();
        return false;
    }
}

bool FomeScript::addAgent(QJSValue program, QString name)
{
    try {
        AgentType *at = new AgentType();
        at->setupSTP(program, name);
        ForestManagementEngine::instance()->addAgent(at);
        return true;
    } catch (const IException &e) {
        qCWarning(abe) << e.message();
        return false;
    }

}

bool FomeScript::runActivity(int stand_id, QString activity)
{
    // find stand
    FMStand *stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand)
        return false;
    if (!stand->stp())
        return false;
    Activity *act = stand->stp()->activity(activity);
    if (!act)
        return false;
    // run the activity....
    qCDebug(abe) << "running activity" << activity << "for stand" << stand_id;
    return act->execute(stand);
}


} // namespace
