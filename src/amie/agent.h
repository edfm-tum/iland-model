#ifndef AGENT_H
#define AGENT_H

namespace AMIE {


class AgentType; // forward

/** The Agent is the core element of the agent based forest management model and simulates
 *  a foresters decisions. */
class Agent
{
public:
    Agent(const AgentType *type);

private:
    // link to the base agent type
    const AgentType *mType;
    // agent properties
    double mKnowledge;
    double mEconomy;
    double mExperimentation;
    double mAltruism;
    double mRisk;

};

} // namespace
#endif // AGENT_H
