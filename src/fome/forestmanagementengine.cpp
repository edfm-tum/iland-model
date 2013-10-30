#include "fome_global.h"
#include "globalsettings.h"


#include "forestmanagementengine.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstand.h"
#include "agent.h"
#include "agenttype.h"
#include "fomescript.h"
#include "scriptglobal.h"
#include "fomescript.h"

#include "debugtimer.h"

/** @class ForestManagementEngine
*/

ForestManagementEngine *ForestManagementEngine::singleton_fome_engine = 0;
ForestManagementEngine::ForestManagementEngine()
{
    mScriptBridge = 0;
    singleton_fome_engine = this;
}

ForestManagementEngine::~ForestManagementEngine()
{
    if (mScriptBridge)
        delete mScriptBridge;
    singleton_fome_engine = 0;
}

void ForestManagementEngine::setup()
{
    // load test data...
    ScriptGlobal::setupGlobalScripting(); // general iLand scripting helper functions and such
    mKnowledgeBase.setup("E:/Daten/iLand/modeling/abm/knowledge_base/test");

    if (mScriptBridge)
        delete mScriptBridge;
    mScriptBridge = new FomeScript;
    mScriptBridge->setupScriptEnvironment();
}

void ForestManagementEngine::clear()
{
    qDeleteAll(mUnitStandMap); // deletes the stands (not the keys)
    mUnitStandMap.clear();
    qDeleteAll(mUnits); // deletes the units
    mUnits.clear();
    qDeleteAll(mAgents);
    mAgents.clear();
    qDeleteAll(mAgentTypes);
    mAgentTypes.clear();
}

void ForestManagementEngine::evaluateActivities()
{
    // loop over all stands and evaluate activities
     QMultiMap<FMUnit*, FMStand*>::const_iterator it = mUnitStandMap.begin();
     while (it != mUnitStandMap.end()) {
         mKnowledgeBase.evaluate(it.value());
         it++;
     }
}

void ForestManagementEngine::test()
{
    // test code
    try {
        //Activity::setVerbose(true);
        // setup the activities and the javascript environment...
        setup();

    } catch (const IException &e) {
        qDebug() << "An error occured:" << e.message();
    }

    // setup some units and stands....
    DebugTimer t;
    AgentType *agent_type = new AgentType();
    mAgentTypes.append(agent_type);

    for (int i=0;i<100;++i) {
        // one agent per unit, and one type for all agents
        Agent *agent = new Agent(agent_type);
        mAgents.append(agent);

        FMUnit *unit = new FMUnit(agent);
        mUnits.append(unit);

        // stands
        for (int j=0;j<100;j++) {
            FMStand *stand = new FMStand(unit, i*1000+j);
            mUnitStandMap.insert(unit, stand);
        }
    }
    // just a demo: evaluate all stands
    qDebug() << "evaluating stands....";
    try {

        foreach(const FMStand *stand, mUnitStandMap) {
            mKnowledgeBase.evaluate(stand);
        }

    }  catch (const IException &e) {
        qDebug() << "An error occured in evaluating stands:" << e.message();
    }


    qDebug() << "evaluating finished";

    clear();
}

QJSEngine *ForestManagementEngine::scriptEngine()
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();
}
