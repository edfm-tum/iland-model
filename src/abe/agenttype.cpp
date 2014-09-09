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
        qCDebug(abeSetup) << "setup of agent" << agent_name << mSTP.size() << "links to STPs established.";


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
