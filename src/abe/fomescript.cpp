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

#include "global.h"
#include "abe_global.h"
#include "fomescript.h"

#include "forestmanagementengine.h"
#include "fmstp.h"
#include "agenttype.h"
#include "agent.h"
#include "fmtreelist.h"
#include "scheduler.h"
#include "fmstp.h"

#include "actplanting.h"

// iLand main includes
#include "species.h"

namespace ABE {

/** @class FomeScript
    @ingroup abe
    The FomeScript class is visible to Javascript via the 'fmengine' object. The main functions of ABE are available through this class.


  */


QString FomeScript::mInvalidContext = "S---";
ActivityFlags ActivityObj::mEmptyFlags;

FomeScript::FomeScript(QObject *parent) :
    QObject(parent)
{
    mStandObj = 0;
    mUnitObj = 0;
    mSimulationObj = 0;
    mActivityObj = 0;
    mSchedulerObj = 0;
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
    mUnitObj = new UnitObj;
    QJSValue site_value = ForestManagementEngine::scriptEngine()->newQObject(mUnitObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("unit", site_value);

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

    // options of the STP
    mSTPObj = new STPObj;
    QJSValue stp_value = ForestManagementEngine::scriptEngine()->newQObject(mSTPObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("stp", stp_value);

    // scheduler options
    mSchedulerObj = new SchedulerObj;
    QJSValue scheduler_value = ForestManagementEngine::scriptEngine()->newQObject(mSchedulerObj);
    ForestManagementEngine::scriptEngine()->globalObject().setProperty("scheduler", scheduler_value);

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
    br->mUnitObj->setStand(stand);
    br->mActivityObj->setStand(stand);
    br->mSchedulerObj->setStand(stand);
    br->mSTPObj->setSTP(stand);
    if (stand && stand->trace())
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

QString FomeScript::JStoString(QJSValue value)
{
    if (value.isArray() || value.isObject()) {
        QJSValue fun = ForestManagementEngine::scriptEngine()->evaluate("(function(a) { return JSON.stringify(a); })");
        QJSValue result = fun.call(QJSValueList() << value);
        return result.toString();
    } else
        return value.toString();

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
    QString msg = JStoString(value);
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

bool FomeScript::runActivityEvaluate(int stand_id, QString activity)
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
    qCDebug(abe) << "running evaluate of activity" << activity << "for stand" << stand_id;
    return act->evaluate(stand);

}

bool FomeScript::runAgent(int stand_id, QString function)
{
    // find stand
    FMStand *stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand)
        return false;

    setExecutionContext(stand, true); // true: add also agent as 'agent'

    QJSValue val;
    QJSValue agent_type = stand->unit()->agent()->type()->jsObject();
    if (agent_type.property(function).isCallable()) {
        val = agent_type.property(function).callWithInstance(agent_type);
        qCDebug(abe) << "running agent-function" << function << "for stand" << stand_id << ":" << val.toString();
    } else {
       qCDebug(abe) << "function" << function << "is not a valid function of agent-type" << stand->unit()->agent()->type()->name();
    }

    return true;


}

bool FomeScript::isValidStand(int stand_id)
{
    FMStand *stand = ForestManagementEngine::instance()->stand(stand_id);
    if (stand)
        return true;

    return false;
}

QStringList FomeScript::standIds()
{
    return ForestManagementEngine::instance()->standIds();
}

QJSValue FomeScript::activity(QString stp_name, QString activity_name)
{

    FMSTP *stp = ForestManagementEngine::instance()->stp(stp_name);
    if (!stp) {
        qCDebug(abe) << "fmengine.activty: invalid stp" << stp_name;
        return QJSValue();
    }

    Activity *act = stp->activity(activity_name);
    if (!act) {
        qCDebug(abe) << "fmengine.activty: activity" << activity_name << "not found in stp:" << stp_name;
        return QJSValue();
    }

    int idx = stp->activityIndex(act);
    ActivityObj *ao = new ActivityObj(0, act, idx);
    QJSValue value = ForestManagementEngine::scriptEngine()->newQObject(ao);
    return value;

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

int FomeScript::levelIndex(const QString &level_label)
{
    if (level_label=="low") return 1;
    if (level_label=="medium") return 2;
    if (level_label=="high") return 3;
    return -1;
}

const QString FomeScript::levelLabel(const int level_index)
{
    switch (level_index) {
    case 1: return QStringLiteral("low");
    case 2: return QStringLiteral("medium");
    case 3: return QStringLiteral("high");
    }
    return QStringLiteral("invalid");
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
    if (mStand)
        return mStand->U();
    throwError("U");
    return -1.;
}

QString StandObj::speciesComposition() const
{
    int index = mStand->targetSpeciesIndex();
    return mStand->unit()->agent()->type()->speciesCompositionName(index);

}

QString StandObj::thinningIntensity() const
{
    int t = mStand->thinningIntensity();
    return FomeScript::levelLabel(t);

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
    return mActivity? mActivity->name() : QStringLiteral("undefined");
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


    qCDebug(abe) << "ActivityObj:flags: invalid access of flags! stand: " << mStand << "activity-index:" << mActivityIndex;
    return mEmptyFlags;
}

bool UnitObj::agentUpdate(QString what, QString how, QString when)
{
    AgentUpdate::UpdateType type = AgentUpdate::label(what);
    if (type == AgentUpdate::UpdateInvalid)
        qCDebug(abe) << "unit.agentUpdate: invalid 'what':" << what;

    AgentUpdate update;
    update.setType( type );

    // how
    int idx = FomeScript::levelIndex(how);
    if (idx>-1)
        update.setValue( QString::number(idx));
    else
        update.setValue( how );

    // when
    bool ok;
    int age = when.toInt(&ok);
    if (ok)
        update.setTimeAge(age);
    else
        update.setTimeActivity(when);

    mStand->unit()->agent()->type()->addAgentUpdate( update, const_cast<FMUnit*>(mStand->unit()) );
    qCDebug(abe) << "Unit::agentUpdate:" << update.dump();
    return true;
}

QString UnitObj::harvestMode() const
{
    return mStand->unit()->harvestMode();
}

QString UnitObj::speciesComposition() const
{
    int index = mStand->unit()->targetSpeciesIndex();
    return mStand->unit()->agent()->type()->speciesCompositionName(index);
}

double UnitObj::U() const
{
    return mStand->U();
}

QString UnitObj::thinningIntensity() const
{
    int t = mStand->unit()->thinningIntensity();
    return FomeScript::levelLabel(t);
}

double UnitObj::MAIChange() const
{
    // todo
    return mStand->unit()->annualIncrement();
}

double UnitObj::MAILevel() const
{
    return mStand->unit()->averageMAI();
}

double UnitObj::landscapeMAI() const
{
    // hacky way of getting a MAI on landscape level
    double total_area = 0.;
    double total_mai = 0.;
    const QVector<FMUnit*> &units = ForestManagementEngine::instance()->units();
    for (int i=0;i<units.size();++i) {
        total_area += units[i]->area();
        total_mai += units[i]->annualIncrement()*units[i]->area();
    }
    if (total_area>0.)
        return total_mai / total_area;
    else
        return 0.;
}

double UnitObj::mortalityChange() const
{
    return 1; // todo
}

double UnitObj::mortalityLevel() const
{
    return 1; // todo

}

double UnitObj::regenerationChange() const
{
    return 1; // todo

}

double UnitObj::regenerationLevel() const
{
    return 1; // todo

}

void SchedulerObj::dump() const
{
    if (!mStand || !mStand->unit() || !mStand->unit()->constScheduler())
        return;
    mStand->unit()->constScheduler()->dump();
}

bool SchedulerObj::enabled()
{
    if (!mStand) return false;
    const SchedulerOptions &opt = mStand->unit()->agent()->schedulerOptions();
    return opt.useScheduler;
}

void SchedulerObj::setEnabled(bool is_enabled)
{
    if (!mStand)
        return;
    SchedulerOptions &opt = const_cast<SchedulerOptions &>(mStand->unit()->agent()->schedulerOptions());
    opt.useScheduler = is_enabled;

}

double SchedulerObj::harvestIntensity()
{
    if (!mStand) return false;
    const SchedulerOptions &opt = mStand->unit()->agent()->schedulerOptions();
    return opt.harvestIntensity;
}

void SchedulerObj::setHarvestIntensity(double new_intensity)
{
    if (!mStand)
        return;
    SchedulerOptions &opt = const_cast<SchedulerOptions &>(mStand->unit()->agent()->schedulerOptions());
    opt.harvestIntensity = new_intensity;

}

double SchedulerObj::useSustainableHarvest()
{
    if (!mStand) return false;
    const SchedulerOptions &opt = mStand->unit()->agent()->schedulerOptions();
    return opt.useSustainableHarvest;
}

void SchedulerObj::setUseSustainableHarvest(double new_level)
{
    if (!mStand)
        return;
    SchedulerOptions &opt = const_cast<SchedulerOptions &>(mStand->unit()->agent()->schedulerOptions());
    opt.useSustainableHarvest = new_level;

}

double SchedulerObj::maxHarvestLevel()
{
    if (!mStand) return false;
    const SchedulerOptions &opt = mStand->unit()->agent()->schedulerOptions();
    return opt.maxHarvestLevel;
}

void SchedulerObj::setMaxHarvestLevel(double new_harvest_level)
{
    if (!mStand)
        return;
    SchedulerOptions &opt = const_cast<SchedulerOptions &>(mStand->unit()->agent()->schedulerOptions());
    opt.maxHarvestLevel = new_harvest_level;
}

void STPObj::setSTP(FMStand *stand)
{
    if (stand && stand->stp()) {
        mSTP = stand->stp();
        mOptions = mSTP->JSoptions();
    } else {
        mOptions = QJSValue();
        mSTP = 0;
    }
}

QString STPObj::name()
{
    if (mSTP)
        return mSTP->name();
    else
        return "undefined";
}


} // namespace
