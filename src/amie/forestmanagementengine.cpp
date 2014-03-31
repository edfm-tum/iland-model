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
#include "threadrunner.h"

Q_LOGGING_CATEGORY(abe, "abe")

Q_LOGGING_CATEGORY(abeSetup, "abe.setup")

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

const MapGrid *ForestManagementEngine::standGrid()
{
    return GlobalSettings::instance()->model()->standGrid();
}


void ForestManagementEngine::setupScripting()
{
    // setup the AMIE system
    const XmlHelper &xml = GlobalSettings::instance()->settings();

    ScriptGlobal::setupGlobalScripting(); // general iLand scripting helper functions and such

    // the link between the scripting and the C++ side of AMIE
    if (mScriptBridge)
        delete mScriptBridge;
    mScriptBridge = new FomeScript;
    mScriptBridge->setupScriptEnvironment();

    QString file_name = GlobalSettings::instance()->path(xml.value("model.management.amie.file"));
    QString code = Helper::loadTextFile(file_name);
    qCDebug(abeSetup) << "Loading script file" << file_name;
    QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate(code,file_name);
    if (result.isError()) {
        int lineno = result.property("lineNumber").toInt();
        QStringList code_lines = code.replace('\r', "").split('\n'); // remove CR, split by LF
        QString code_part;
        for (int i=std::max(0, lineno - 5); i<std::min(lineno+5, code_lines.count()); ++i)
            code_part.append(QString("%1: %2 %3\n").arg(i).arg(code_lines[i]).arg(i==lineno?"  <---- [ERROR]":""));
        qCDebug(abeSetup) << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.property("lineNumber").toInt() << ":" << result.toString() << ":\n" << code_part;
    }

}

AgentType *ForestManagementEngine::agentType(const QString &name)
{
    for (int i=0;i<mAgentTypes.count();++i)
        if (mAgentTypes[i]->name()==name)
            return mAgentTypes[i];
    return 0;
}

void ForestManagementEngine::setup()
{
    QLoggingCategory::setFilterRules("abe.debug=true\n" \
                                     "abe.setup.debug=true"); // enable *all*

    clear();
    const XmlHelper &xml = GlobalSettings::instance()->settings();

    // (1) setup the scripting environment and load all the javascript code
    setupScripting();


    if (!GlobalSettings::instance()->model())
        throw IException("No model created.... invalid operation.");
    // (2) spatial data (stands, units, ...)
    const MapGrid *stand_grid = GlobalSettings::instance()->model()->standGrid();

    if (stand_grid==NULL || stand_grid->isValid()==false)
        throw IException("The AMIE management model requires a valid stand grid.");

    QString data_file_name = GlobalSettings::instance()->path(xml.value("model.management.amie.agentDataFile"));
    CSVFile data_file(data_file_name);
    if (data_file.rowCount()==0)
        throw IException(QString("Stand-Initialization: the standDataFile file %1 is empty or missing!").arg(data_file_name));
    int ikey = data_file.columnIndex("id");
    int iunit = data_file.columnIndex("unit");
    int iagent = data_file.columnIndex("agent");
    if (ikey<0 || iunit<0 || iagent<0)
        throw IException("setup AMIE agentDataFile: one (or more) of the required columns 'id','unit', 'agent' not available.");

    QList<QString> unit_codes;
    QList<QString> agent_codes;
    for (int i=0;i<data_file.rowCount();++i) {
        int stand_id = data_file.value(i,ikey).toInt();
        if (!stand_grid->isValid(stand_id))
            continue; // skip stands that are not in the map (e.g. when a smaller extent is simulated)
        qCDebug(abeSetup) << "setting up stand" << stand_id;

        // check agents
        QString agent_code = data_file.value(i, iagent).toString();
        Agent *agent=0;
        AgentType *at=0;
        if (!agent_codes.contains(agent_code)) {
            // create the agent / agent type
            at = agentType(agent_code);
            if (!at)
                throw IException(QString("Agent %1 is not set up!").arg(agent_code));
            agent = new Agent(at);
            mAgents.append(agent);
            agent_codes.append(agent_code);
        } else {
            // simplified: one agent for all stands with the same agent....
            agent = mAgents[ agent_codes.indexOf(agent_code) ];
            at = agent->type();

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
            at->addUnit(unit); // add the unit to the list of managed units of the agent
        } else {
            // get unit by id ... in this case we have the same order of appending values
            unit = mUnits[unit_codes.indexOf(unit_id)];
        }

        // create stand
        FMStand *stand = new FMStand(unit,stand_id);

        mUnitStandMap.insertMulti(unit,stand);
        mStands.append(stand);

    }
    // set up the stand grid (visualizations)...
    // set up a hash for helping to establish stand-id <-> fmstand-link
    QHash<int, FMStand*> stand_hash;
    for (int i=0;i<mStands.size(); ++i)
        stand_hash[mStands[i]->id()] = mStands[i];

    mFMStandGrid.setup(standGrid()->grid().cellsize(), standGrid()->grid().sizeX(), standGrid()->grid().sizeY() );
    mFMStandGrid.initialize(0);
    FMStand **fm = mFMStandGrid.begin();
    for (int *p = standGrid()->grid().begin(); p!=standGrid()->grid().end(); ++p, ++fm)
        *fm = *p<0?0:stand_hash[*p];

    mStandLayers.setGrid(mFMStandGrid);
    mStandLayers.clearClasses();
    mStandLayers.registerLayers();

    // now initialize the agents....
    foreach(AgentType *at, mAgentTypes)
        at->setup();
    qCDebug(abeSetup) << "ABE setup complete." << mUnitStandMap.size() << "stands on" << mUnits.count() << "units, managed by" << mAgents.size() << "agents.";

}

void ForestManagementEngine::clear()
{
    qDeleteAll(mStands); // delete the stands
    mStands.clear();
    qDeleteAll(mUnits); // deletes the units
    mUnits.clear();
    mUnitStandMap.clear();

    qDeleteAll(mAgents);
    mAgents.clear();
    qDeleteAll(mAgentTypes);
    mAgentTypes.clear();
    qDeleteAll(mSTP);
    mSTP.clear();
    mCurrentYear = 0;
}


FMUnit *nc_execute_unit(FMUnit *unit)
{
    //qDebug() << "called for unit" << unit;
    const QMultiMap<FMUnit*, FMStand*> &stand_map = ForestManagementEngine::instance()->stands();
    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stand_map.constFind(unit);
    while (it!=stand_map.constEnd() && it.key()==unit) {
        it.value()->execute();
        ++it;
    }
    return unit;
}

/// this is the main function of the forest management engine.
/// the function is called every year.
void ForestManagementEngine::run(int debug_year)
{
    if (debug_year>-1) {
        mCurrentYear++;
    } else {
        mCurrentYear = GlobalSettings::instance()->currentYear();
    }
    // now re-evaluate stands
    if (FMSTP::verbose()) qCDebug(abe) << "ForestManagementEngine: run year" << mCurrentYear;

    GlobalSettings::instance()->model()->threadExec().run(nc_execute_unit, mUnits);

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
    QString file_name = "E:/Daten/iLand/modeling/abm/knowledge_base/test/test_stp.js";
    QString code = Helper::loadTextFile(file_name);
    QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate(code,file_name);
    if (result.isError()) {
        int lineno = result.property("lineNumber").toInt();
        QStringList code_lines = code.replace('\r', "").split('\n'); // remove CR, split by LF
        QString code_part;
        for (int i=std::max(0, lineno - 5); i<std::min(lineno+5, code_lines.count()); ++i)
            code_part.append(QString("%1: %2 %3\n").arg(i).arg(code_lines[i]).arg(i==lineno?"  <---- [ERROR]":""));
        qDebug() << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.property("lineNumber").toInt() << ":" << result.toString() << ":\n" << code_part;
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

    setup();
    qDebug() << "finished";

}

QJSEngine *ForestManagementEngine::scriptEngine()
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();
}

FMSTP *ForestManagementEngine::stp(QString stp_name) const
{
    for (QVector<FMSTP*>::const_iterator it = mSTP.constBegin(); it!=mSTP.constEnd(); ++it)
        if ( (*it)->name() == stp_name )
            return *it;
    return 0;
}

FMStand *ForestManagementEngine::stand(int stand_id) const
{
    for (QVector<FMStand*>::const_iterator it=mStands.constBegin(); it!=mStands.constEnd(); ++it)
        if ( (*it)->id() == stand_id)
            return *it;
    return 0;
}



} // namespace
