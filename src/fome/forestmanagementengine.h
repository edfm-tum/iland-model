#ifndef FORESTMANAGEMENTENGINE_H
#define FORESTMANAGEMENTENGINE_H
#include <QMultiMap>

#include "knowledgebase.h"
class FMUnit; // forward
class FMStand; // forward
class Agent; // forward
class AgentType; // forward

class QJSEngine; // forward

/// the FOrestManagementEngine is the container for the agent based forest management engine.
class ForestManagementEngine
{
public:
    // life cycle
    ForestManagementEngine();
    void setup();
    void clear(); ///< delete all objects and free memory
    // properties

    // functions
    void evaluateActivities();
    void test();

    /// access to the "global" Javascript engine
    static QJSEngine *scriptEngine();
private:
    // the knowledge base is the collection of silvicultural treatments
    KnowledgeBase mKnowledgeBase;

    // forest management units
    QVector<FMUnit*> mUnits; ///< container for forest management units
    // mapping of stands to units
    QMultiMap<FMUnit*, FMStand*> mUnitStandMap;

    // agents
    QVector<AgentType*> mAgentTypes; ///< collection of agent types
    QVector<Agent*> mAgents; ///< collection of all agents (individuals)
};

#endif // FORESTMANAGEMENTENGINE_H
