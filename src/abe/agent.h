#ifndef AGENT_H
#define AGENT_H

#include <QJSValue>

namespace ABE {


class AgentType; // forward

/** The Agent is the core element of the agent based forest management model and simulates
 *  a foresters decisions. */
class Agent
{
public:
    Agent(AgentType *type, QJSValue js);
    AgentType *type() const {return mType; }
    QString name() const { return mName; }
    void setName(const QString &name);
    QJSValue jsAgent() const { return mJSAgent; }
    // agent properties
    double useSustainableHarvest() const;

private:
    static int mAgentsCreated;
    // link to the base agent type
    AgentType *mType;
    // the javascript object representing the agent:
    QJSValue mJSAgent;
    // agent properties
    QString mName;
    double mKnowledge;
    double mEconomy;
    double mExperimentation;
    double mAltruism;
    double mRisk;

};

} // namespace
#endif // AGENT_H
