#include "global.h"
#include "abe_global.h"
#include "fomescript.h"

#include "forestmanagementengine.h"
#include "fmstp.h"
#include "agenttype.h"
#include "agent.h"
#include "fmtreelist.h"

#include "actplanting.h"

// iLand main includes
#include "species.h"

namespace ABE {

QString FomeScript::mInvalidContext = "S---";

FomeScript::FomeScript(QObject *parent) :
    QObject(parent)
{
    mStandObj = 0;
    mSiteObj = 0;
    mSimulationObj = 0;
    mActivityObj = 0;
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

    //access to the current activity
    mActivityObj = new ActivityObj;
    QJSValue activity_value = ForestManagementEngine::scriptEngine()->newQObject(mActivityObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("activity", activity_value);

    // general simulation variables (mainly scenariolevel)
    mTrees = new FMTreeList;
    QJSValue treelist_value = ForestManagementEngine::scriptEngine()->newQObject(mTrees);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("trees", treelist_value);

    // the script object itself
    QJSValue script_value = ForestManagementEngine::scriptEngine()->newQObject(this);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("fmengine", script_value);

}

void FomeScript::setExecutionContext(FMStand *stand, bool add_agent)
{
    FomeScript *br = bridge();
    br->mStand = stand;
    br->mStandObj->setStand(stand);
    br->mTrees->setStand(stand);
    br->mSiteObj->setStand(stand);
    br->mActivityObj->setStand(stand);
    if (stand->trace())
        qCDebug(abe) << br->context() << "Prepared execution context (thread" << QThread::currentThread() << ").";
    if (add_agent) {
        const Agent *ag = stand->unit()->agent();
        ForestManagementEngine::instance()->scriptEngine()->globalObject().setProperty("agent", ag->jsAgent());
    }

}

void FomeScript::setActivity(Activity *act)
{
    FomeScript *br = bridge();
    setExecutionContext(0);
    br->mActivityObj->setActivity(act);
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

int FomeScript::standId() const
{
    if (mStand)
        return mStand->id();
    return -1;
}

void FomeScript::setStandId(int new_stand_id)
{
    FMStand *stand = ForestManagementEngine::instance()->stand(new_stand_id);
    if (!stand) {
        qCDebug(abe) << bridge()->context() << "invalid stand id" << new_stand_id;
        return;
    }
    setExecutionContext(stand);
}

void FomeScript::log(QJSValue value)
{
    QString msg = value.toString();
    qCDebug(abe) << bridge()->context() << msg;
}

void FomeScript::abort(QJSValue message)
{
    log(message);
    ForestManagementEngine::instance()->abortExecution(QString("%1: %2").arg(context()).arg(message.toString()));
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
        ForestManagementEngine::instance()->abortExecution(QString("Error in adding management.\n%1").arg(e.message()));
        return false;
    }
}

bool FomeScript::addAgentType(QJSValue program, QString name)
{
    try {
        AgentType *at = new AgentType();
        at->setupSTP(program, name);
        ForestManagementEngine::instance()->addAgentType(at);
        return true;
    } catch (const IException &e) {
        qCWarning(abe) << e.message();
        ForestManagementEngine::instance()->abortExecution(QString("Error in adding agent type definition.\n%1").arg(e.message()));
        return false;
    }

}

QJSValue FomeScript::addAgent(QString agent_type, QString agent_name)
{
    try {
    // find the agent type
    AgentType *at = ForestManagementEngine::instance()->agentType(agent_type);
    if (!at) {
        abort(QString("fmengine.addAgent: invalid 'agent_type': '%1'").arg(agent_type));
        return QJSValue();
    }
    Agent *ag = at->createAgent(agent_name);
    return ag->jsAgent();
    } catch (const IException &e) {
        qCWarning(abe) << e.message();
        ForestManagementEngine::instance()->abortExecution(QString("Error in adding agent definition.\n%1").arg(e.message()));
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

bool FomeScript::runAgent(int stand_id, QString function)
{
    // find stand
    FMStand *stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand)
        return false;

    setExecutionContext(stand, true); // true: add also agent as 'agent'
    QJSValue val = ForestManagementEngine::scriptEngine()->evaluate(QString("%1()").arg(function));
    qCDebug(abe) << "running agent-function" << function << "for stand" << stand_id << ":" << val.toString();
    return true;


}

void FomeScript::runPlanting(int stand_id, QJSValue planting_item)
{
    FMStand *stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand) {
        qCWarning(abe) << "runPlanting: stand not found" << stand_id;
        return;
    }

    ActPlanting::runSinglePlantingItem(stand, planting_item);


}

QString StandObj::speciesId(int index) const
{
    if (index>=0 && index<nspecies()) return mStand->speciesData(index).species->id(); else return "error";
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

QJSValue StandObj::agent()
{
    if (mStand && mStand->unit()->agent())
        return mStand->unit()->agent()->jsAgent();
    else
        throwError("get agent of the stand failed.");
    return QJSValue();
}

void StandObj::setAbsoluteAge(double arg)
{
    if (!mStand) {
        throwError("set absolute age"); return; }
    mStand->setAbsoluteAge(arg);
}

bool StandObj::trace() const
{
    if (!mStand) { throwError("trace"); return false; }
    return mStand->trace();
}

void StandObj::setTrace(bool do_trace)
{
    if (!mStand) { throwError("trace"); }
    mStand->setProperty("trace", QJSValue(do_trace));
}

int StandObj::timeSinceLastExecution() const
{
    if (mStand)
        return ForestManagementEngine::instance()->currentYear() - mStand->lastExecution();
    throwError("timeSinceLastExecution");
    return -1;
}

QString StandObj::lastActivity() const
{
    if (mStand->lastExecutedActivity())
        return mStand->lastExecutedActivity()->name();
    return QString();
}

double StandObj::rotationLength() const
{
    if (mStand && mStand->stp())
        return mStand->stp()->rotationLength();
    throwError("rotationLength");
    return -1.;
}

void StandObj::throwError(QString msg) const
{
    FomeScript::bridge()->abort(QString("Error while accessing 'stand': no valid execution context. Message: %1").arg(msg));
}

/* ******************** */

bool ActivityObj::enabled() const
{
    return flags().enabled();
}

QString ActivityObj::name() const
{
    return mActivity->name();
}

void ActivityObj::setEnabled(bool do_enable)
{
    flags().setEnabled(do_enable);
}

ActivityFlags &ActivityObj::flags() const
{
    // refer to a specific  activity of the stand (as returned by stand.activity("xxx") )
    if (mStand && mActivityIndex>-1)
        return mStand->flags(mActivityIndex);
    // refer to the *current* activity (the "activity" variable)
    if (mStand && !mActivity)
        return mStand->currentFlags();
    // during setup of activites (onCreate-handler)
    if (!mStand && mActivity)
        return mActivity->mBaseActivity;


    throw IException("ActivityObj:flags: invalid access of flags!");
}


} // namespace
