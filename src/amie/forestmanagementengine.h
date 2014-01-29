#ifndef FORESTMANAGEMENTENGINE_H
#define FORESTMANAGEMENTENGINE_H
#include <QMultiMap>
#include <QVector>

#include "knowledgebase.h"

class QJSEngine; // forward

namespace AMIE {

class FMUnit; // forward
class FMStand; // forward
class FMSTP; // forward
class Agent; // forward
class AgentType; // forward
class FomeScript; // forward

/// the FOrestManagementEngine is the container for the agent based forest management engine.
class ForestManagementEngine
{
public:
    // life cycle
    ForestManagementEngine();
    ~ForestManagementEngine();
    // engine instance
    static ForestManagementEngine *instance() { if (singleton_fome_engine) return singleton_fome_engine; singleton_fome_engine = new ForestManagementEngine; return singleton_fome_engine; }
    void setup();
    void clear(); ///< delete all objects and free memory
    // properties
    /// access to the "global" Javascript engine
    static QJSEngine *scriptEngine();
    FomeScript *scriptBridge() const {return mScriptBridge; }

    void addSTP(FMSTP* stp) { mSTP.push_back(stp);}
    // functions
    /// evalaute forest management activities and select fitting activities for each forest stand
    void evaluateActivities();
    void test();
    void test_old();


private:
    static ForestManagementEngine *singleton_fome_engine;
    // the knowledge base is the collection of silvicultural treatments ???
    KnowledgeBase mKnowledgeBase;

    QVector<FMSTP*> mSTP;

    // scripting bridge (accessing model properties from javascript)
    FomeScript *mScriptBridge;

    // forest management units
    QVector<FMUnit*> mUnits; ///< container for forest management units
    // mapping of stands to units
    QMultiMap<FMUnit*, FMStand*> mUnitStandMap;

    // agents
    QVector<AgentType*> mAgentTypes; ///< collection of agent types
    QVector<Agent*> mAgents; ///< collection of all agents (individuals)
};


} // namespace
#endif // FORESTMANAGEMENTENGINE_H
