#include "agent.h"
#include "agenttype.h"
#include "scheduler.h"
namespace ABE {

int Agent::mAgentsCreated = 0;

Agent::Agent(AgentType *type, QJSValue js)
{
    mType = type;
    mJSAgent = js;
    mAgentsCreated++;
    mName = QString("agent_%1").arg(mAgentsCreated);
}

void Agent::setName(const QString &name)
{
    mName = name;
    mJSAgent.setProperty("name", name);
}

double Agent::useSustainableHarvest() const
{
    return mType->schedulerOptions().useSustainableHarvest;
}

} // namespace
