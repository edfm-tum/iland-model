#ifndef AGENT_H
#define AGENT_H

#include <QJSValue>
#include "scheduler.h"

namespace ABE {


class AgentType; // forward
class FMUnit; // forward

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
    const SchedulerOptions &schedulerOptions() const { return mSchedulerOptions; }

    /// add a unit to the list of managed units
    void addUnit(FMUnit *unit) {mUnits.push_back(unit);}


    // agent properties
    double useSustainableHarvest() const;

    void setup();
private:
    static int mAgentsCreated;
    // link to the base agent type
    AgentType *mType;
    // the javascript object representing the agent:
    QJSValue mJSAgent;
    SchedulerOptions mSchedulerOptions; ///< agent specific scheduler options
    QVector<FMUnit*> mUnits; ///< list of units managed by the agent

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
