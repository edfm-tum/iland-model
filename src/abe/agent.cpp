#include "agent.h"
#include "agenttype.h"
#include "scheduler.h"
namespace ABE {


Agent::Agent(AgentType *type)
{
    mType = type;
}

double Agent::useSustainableHarvest() const
{
    return mType->schedulerOptions().useSustainableHarvest;
}

} // namespace
