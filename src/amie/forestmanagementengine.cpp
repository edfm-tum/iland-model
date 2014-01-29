#include "amie_global.h"
#include "globalsettings.h"


#include "forestmanagementengine.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstand.h"
#include "fmstp.h"
#include "agent.h"
#include "agenttype.h"
#include "fomescript.h"
#include "scriptglobal.h"
#include "fomescript.h"

#include "debugtimer.h"

// general iLand stuff
#include "xmlhelper.h"
#include "csvfile.h"
#include "model.h"
#include "mapgrid.h"
#include "helper.h"


namespace AMIE {

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
    clear();
    if (mScriptBridge)
        delete mScriptBridge;
    singleton_fome_engine = 0;
}

void ForestManagementEngine::setup()
{
    clear();

    // setup the AMIE system
    const XmlHelper &xml = GlobalSettings::instance()->settings();

    // (1) the knowledge base: the (javascript) definition of forest management activities
    ScriptGlobal::setupGlobalScripting(); // general iLand scripting helper functions and such
    mKnowledgeBase.setup(GlobalSettings::instance()->path(xml.value("model.management.setup.knowledgeBaseDir")));

    // (2) the link between the scripting and the C++ side of AMIE
    if (mScriptBridge)
        delete mScriptBridge;
    mScriptBridge = new FomeScript;
    mScriptBridge->setupScriptEnvironment();

    // (3) spatial data (stands, units, ...)
    const MapGrid *stand_grid = GlobalSettings::instance()->model()->standGrid();
    if (stand_grid==NULL || stand_grid->isValid()==false)
        throw IException("The AMIE management model requires a valid stand grid.");

    QString data_file_name = GlobalSettings::instance()->path(xml.value("model.management.setup.standDataFile"));
    CSVFile data_file(data_file_name);
    if (data_file.rowCount()==0)
        throw IException(QString("Stand-Initialization: the standDataFile file %1 is empty or missing!").arg(data_file_name));
    int ikey = data_file.columnIndex("id");
    int iunit = data_file.columnIndex("unit");
    int iagent = data_file.columnIndex("agent");
    if (ikey<0 || iunit<0 || iagent<0)
        throw IException("setup AMIE standDataFile: one (or more) of the required columns 'id','unit', 'agent' not available.");

    QList<QString> unit_codes;
    QList<QString> agent_codes;
    for (int i=0;i<data_file.rowCount();++i) {
        int stand_id = data_file.value(i,ikey).toInt();
        if (!stand_grid->isValid(stand_id))
            continue; // skip stands that are not in the map (e.g. when a smaller extent is simulated)
        qDebug() << "setting up stand" << stand_id;

        // check agents
        QString agent_code = data_file.value(i, iagent).toString();
        Agent *agent=0;
        if (!agent_codes.contains(agent_code)) {
            // create the agent / agent type
            AgentType *at = new AgentType;
            agent = new Agent(at);
            mAgentTypes.append(at);
            mAgents.append(agent);
            agent_codes.append(agent_code);
        } else {
            // simplified: one agent for all stands with the same agent....
            agent = mAgents[ agent_codes.indexOf(agent_code) ];
        }

        // check units
        QString unit_id = data_file.value(i, iunit).toString();
        FMUnit *unit = 0;
        if (!unit_codes.contains(unit_id)) {
            // create the unit
            unit = new FMUnit(agent);
            unit->setId(unit_id);
            mUnits.append(unit);
            unit_codes.append(unit_id);
        } else {
            // get unit by id ... in this case we have the same order of appending values
            unit = mUnits[unit_codes.indexOf(unit_id)];
        }

        // create stand
        FMStand *stand = new FMStand(unit,stand_id);

        mUnitStandMap.insertMulti(unit,stand);

    }
    qDebug() << "AMIE setup complete." << mUnitStandMap.size() << "stands on" << mUnits.count() << "units, managed by" << mAgents.size() << "agents.";


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
    qDeleteAll(mSTP);
    mSTP.clear();
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

void ForestManagementEngine::test_old()
{
    // test code
    try {
        //Activity::setVerbose(true);
        // setup the activities and the javascript environment...
        GlobalSettings::instance()->resetScriptEngine(); // clear the script
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
        for (int j=0;j<1000;j++) {
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

void ForestManagementEngine::test()
{
    // test code
    try {
        //Activity::setVerbose(true);
        // setup the activities and the javascript environment...
        GlobalSettings::instance()->resetScriptEngine(); // clear the script
        ScriptGlobal::setupGlobalScripting(); // general iLand scripting helper functions and such
        if (mScriptBridge)
            delete mScriptBridge;
        mScriptBridge = new FomeScript;
        mScriptBridge->setupScriptEnvironment();

        //setup();

    } catch (const IException &e) {
        qDebug() << "An error occured:" << e.message();
    }
    QString code = Helper::loadTextFile("E:/Daten/iLand/modeling/abm/knowledge_base/test/test_stp.js");
    QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate(code);
    if (result.isError()) {
        qDebug() << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.toString();
    }


//    try {
//        qDebug() << "*** test 1 ***";
//        FMSTP stp;
//        stp.setVerbose(true);
//        stp.setup(GlobalSettings::instance()->scriptEngine()->globalObject().property("stp"), "stp");
//        stp.dumpInfo();

//    } catch (const IException &e) {
//        qDebug() << "An error occured:" << e.message();
//    }
//    try {
//        qDebug() << "*** test 2 ***";
//        FMSTP stp2;
//        stp2.setVerbose(true);
//        stp2.setup(GlobalSettings::instance()->scriptEngine()->globalObject().property("degenerated"), "degenerated");
//        stp2.dumpInfo();
//    } catch (const IException &e) {
//        qDebug() << "An error occured:" << e.message();
//    }

    // dump all objects:
    foreach(FMSTP *stp, mSTP)
        stp->dumpInfo();

    qDebug() << "finished";

}

QJSEngine *ForestManagementEngine::scriptEngine()
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();
}


} // namespace
