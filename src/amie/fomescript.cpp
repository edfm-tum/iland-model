#include "global.h"
#include "amie_global.h"
#include "fomescript.h"
#include "forestmanagementengine.h"
#include "fmstp.h"
#include "agenttype.h"
#include "fmtreelist.h"

namespace AMIE {

QString FomeScript::mInvalidContext = "S---";

FomeScript::FomeScript(QObject *parent) :
    QObject(parent)
{
    mStandObj = 0;
    mSiteObj = 0;
    mSimulationObj = 0;
    mTrees = 0;
    mStand = 0;
}

FomeScript::~FomeScript()
{
    // all objects have script ownership - do not delete here.
//    if (mStandObj) {
//        delete mStandObj;
//        delete mSiteObj;
//        delete mSimulationObj;
//        delete mTrees;
//    }
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

void FomeScript::setExecutionContext(FMStand *stand)
{
    FomeScript *br = bridge();
    br->mStand = stand;
    br->mStandObj->setStand(stand);
    br->mTrees->setStand(stand);
    br->mSiteObj->setStand(stand);
    if (stand->trace())
        qCDebug(abe) << br->context() << "Prepared execution context (thread" << QThread::currentThread() << ").";
}

FomeScript *FomeScript::bridge()
{
    // get the right bridge object (for the right thread??)
    return ForestManagementEngine::instance()->scriptBridge();
}

bool FomeScript::verbose() const
{
    return FMSTP::verbose();
}

void FomeScript::setVerbose(bool arg)
{
    FMSTP::setVerbose(arg);
    qCDebug(abe) << "setting verbose property of ABE to" << arg;
}

void FomeScript::log(QJSValue value)
{
    QString msg = value.toString();
    qCDebug(abe) << bridge()->context() << msg;
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

/// force execution of an activity (outside of the usual execution context, e.g. for debugging)
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

QJSValue StandObj::activity(QString name)
{
    Activity *act = mStand->stp()->activity(name);
    if (!act)
        return QJSValue();

    int idx = mStand->stp()->activityIndex(act);
    ActivityObj *ao = new ActivityObj(mStand, act, idx);
    QJSValue value = ForestManagementEngine::scriptEngine()->newQObject(ao);
    return value;

}

bool StandObj::trace() const
{
    return mStand->trace();
}

void StandObj::setTrace(bool do_trace)
{
    mStand->setProperty("trace", QJSValue(do_trace));
}

/* ******************** */

bool ActivityObj::enabled() const
{
    if (!mStand || mActivityIndex<0) return false;
    return mStand->flags(mActivityIndex).enabled();
}

QString ActivityObj::name() const
{
    return mActivity->name();
}

void ActivityObj::setEnabled(bool do_enable)
{
    if (!mStand || mActivityIndex<0) return;
    mStand->flags(mActivityIndex).setEnabled(do_enable);
}


} // namespace
