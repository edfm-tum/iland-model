#ifndef FORESTMANAGEMENTENGINE_H
#define FORESTMANAGEMENTENGINE_H
#include <QMultiMap>
#include <QVector>

#include "knowledgebase.h"
#include "amiegrid.h"
#include "scheduler.h"

class QJSEngine; // forward
class MapGrid; // forward

namespace AMIE {

class FMUnit; // forward
class FMStand; // forward
class FMSTP; // forward
class Agent; // forward
class AgentType; // forward
class FomeScript; // forward
class Scheduler; // forward

/// the FOrestManagementEngine is the container for the agent based forest management engine.
class ForestManagementEngine
{
public:
    // life cycle
    ForestManagementEngine();
    ~ForestManagementEngine();
    // engine instance (singleton)
    static ForestManagementEngine *instance() { if (singleton_fome_engine) return singleton_fome_engine; singleton_fome_engine = new ForestManagementEngine; return singleton_fome_engine; }
    /// link to stand grid
    static const MapGrid *standGrid();
    void setup();
    void clear(); ///< delete all objects and free memory

    // main function
    void run(int debug_year=-1);

    // properties
    int currentYear() { return mCurrentYear; }
    /// access to the "global" Javascript engine
    static QJSEngine *scriptEngine();
    FomeScript *scriptBridge() const {return mScriptBridge; }
    Scheduler &scheduler() {return mScheduler; }

    void addSTP(FMSTP* stp) { mSTP.push_back(stp);}
    void addAgent(AgentType* at) { mAgentTypes.push_back(at);}
    /// retrieve pointer to stand treatment programme. return 0-pointer if not available.
    FMSTP *stp(QString stp_name) const;
    /// get stand with id 'stand_id'. Return 0 if not found.
    FMStand *stand(int stand_id) const;
    //QVector<FMStand*> stands() const {return mStands; }
    const QMultiMap<FMUnit*, FMStand*> stands() const {return mUnitStandMap; }
    // functions
    /// evalaute forest management activities and select fitting activities for each forest stand
    void evaluateActivities();
    void test();
    void test_old();


private:
    void setupScripting();
    AgentType *agentType(const QString &name);
    static ForestManagementEngine *singleton_fome_engine;
    int mCurrentYear; ///< current year of the simulation (=year of the model)
    Scheduler mScheduler;
    // the knowledge base is the collection of silvicultural treatments ???
    KnowledgeBase mKnowledgeBase;

    QVector<FMSTP*> mSTP;

    // scripting bridge (accessing model properties from javascript)
    FomeScript *mScriptBridge;

    // forest management units
    QVector<FMUnit*> mUnits; ///< container for forest management units
    // mapping of stands to units
    QMultiMap<FMUnit*, FMStand*> mUnitStandMap;
    QVector<FMStand*> mStands;

    // agents
    QVector<AgentType*> mAgentTypes; ///< collection of agent types
    QVector<Agent*> mAgents; ///< collection of all agents (individuals)

    // grids, visuals, etc.
    Grid<FMStand*> mFMStandGrid;
    AMIELayers mStandLayers;
};


} // namespace
#endif // FORESTMANAGEMENTENGINE_H
