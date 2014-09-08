#include "global.h"
#include "abe_global.h"
#include "agenttype.h"
#include "agent.h"
#include "fmstp.h"
#include "forestmanagementengine.h"
#include "fmunit.h"
#include "fmstand.h"
#include "fomescript.h"

#include <QJSEngine>
namespace ABE {


AgentType::AgentType()
{
}

void AgentType::setupSTP(QJSValue agent_code, const QString agent_name)
{
    mName = agent_name;
    mSTP.clear();
    mUnits.clear();
    mJSObj = agent_code;
    if (!agent_code.isObject())
        throw IException(QString("ABE:AgentType:setup: the javascript object for agent '%1' could not be found.").arg(agent_name));
    QJSValue stps = agent_code.property("stp");
    if (!stps.isObject())
        throw IException(QString("ABE:AgentType:setup: the javascript definition of agent '%1' does not have a section for 'stp'.").arg(agent_name));
    QJSValueIterator it(stps);
    while (it.hasNext()) {
        it.next();
        FMSTP *stp = ForestManagementEngine::instance()->stp(it.value().toString());
        if (!stp)
           throw IException(QString("ABE:AgentType:setup: definition of agent '%1': the STP for mixture type '%2': '%3' is not available.").arg(agent_name).arg(it.name()).arg(it.value().toString()));
        mSTP[it.name()] = stp;
    }

    if (FMSTP::verbose())
        qDebug() << "setup of agent" << agent_name << mSTP.size() << "links to STPs established.";

    QJSValue scheduler = agent_code.property("scheduler");
    mSchedulerOptions.setup( scheduler );

}

void AgentType::setup()
{
    FMSTP *stp = mSTP.value("default", 0);
    if (!stp)
        throw IException("AgentType::setup(): default-STP not defined");

    QJSValue onSelect_handler = mJSObj.property("onSelect");

    const QMultiMap<FMUnit*, FMStand*> &stand_map = ForestManagementEngine::instance()->stands();
    foreach (FMUnit *unit, mUnits) {
        QMultiMap<FMUnit*, FMStand*>::const_iterator it = stand_map.constFind(unit);
        while (it!=stand_map.constEnd() && it.key()==unit) {
            FMStand *stand = it.value();
            // check if STP is already assigned. If not, do it now.
            if (!stand->stp()) {
                stand->reload(); // fetch data from iLand ...
                if (onSelect_handler.isCallable()) {
                    FomeScript::setExecutionContext(stand);
                    //QJSValue mix = onSelect_handler.call();
                    QJSValue mix = onSelect_handler.callWithInstance(mJSObj);
                    QString mixture_type = mix.toString();
                    if (!mSTP.contains(mixture_type))
                        throw IException(QString("AgentType::setup(): the selected mixture type '%1' for stand '%2' is not valid for agent '%3'.").arg(mixture_type).arg(stand->id()).arg(mName));
                    stand->setSTP(mSTP[mixture_type]);
                } else {
                    // todo.... some automatic stp selection
                    stand->setSTP(stp);
                }
                stand->initialize(); // run initialization
            }
            ++it;
        }
    }

}

Agent *AgentType::createAgent(QString agent_name)
{
    // call the newAgent function in the javascript object assigned to this agent type
    QJSValue func = mJSObj.property("newAgent");
    if (!func.isCallable())
        throw IException(QString("The agent type '%1' does not have a valid 'newAgent' function.").arg(name()));
    QJSValue result = func.callWithInstance(mJSObj);
    if (result.isError())
        throw IException(QString("calling the 'newAgent' function of agent type '%1' returned with the following error: %2").arg(name()).arg(result.toString()));
    Agent *agent = new Agent(this, result);
    if (!agent_name.isEmpty()) {
        agent->setName(agent_name);
    } else {
        if (result.property("name").isUndefined())
            result.setProperty("name", agent->name()); //  set the auto-generated name also for the JS world
        else
            agent->setName(result.property("name").toString()); // set the JS-name also internally
    }
    ForestManagementEngine::instance()->addAgent(agent);

    return agent;

}

FMSTP *AgentType::stpByName(const QString &name)
{
    if (mSTP.contains(name))
        return mSTP[name];
    else
        return 0;
}


} // namespace
