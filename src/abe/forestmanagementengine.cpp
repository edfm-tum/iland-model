#include "abe_global.h"
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
#include "scheduler.h"

#include "unitout.h"
#include "abestandout.h"
//#include "abestandremovalout.h"

#include "debugtimer.h"

// general iLand stuff
#include "xmlhelper.h"
#include "csvfile.h"
#include "model.h"
#include "mapgrid.h"
#include "helper.h"
#include "threadrunner.h"
#include "outputmanager.h"

#include "tree.h"



Q_LOGGING_CATEGORY(abe, "abe")

Q_LOGGING_CATEGORY(abeSetup, "abe.setup")

namespace ABE {

/** @class ForestManagementEngine
*/

ForestManagementEngine *ForestManagementEngine::singleton_fome_engine = 0;
int ForestManagementEngine::mMaxStandId = -1;
ForestManagementEngine::ForestManagementEngine()
{
    mScriptBridge = 0;
    singleton_fome_engine = this;
    mCancel = false;
    setupOutputs(); // add ABE output definitions
}

ForestManagementEngine::~ForestManagementEngine()
{
    clear();
    // script bridge: script ownership?
    //if (mScriptBridge)
    //    delete mScriptBridge;
    singleton_fome_engine = 0;
}

const MapGrid *ForestManagementEngine::standGrid()
{
    return GlobalSettings::instance()->model()->standGrid();
}


void ForestManagementEngine::setupScripting()
{
    // setup the ABE system
    const XmlHelper &xml = GlobalSettings::instance()->settings();

    ScriptGlobal::setupGlobalScripting(); // general iLand scripting helper functions and such

    // the link between the scripting and the C++ side of ABE
    if (mScriptBridge)
        delete mScriptBridge;
    mScriptBridge = new FomeScript;
    mScriptBridge->setupScriptEnvironment();

    QString file_name = GlobalSettings::instance()->path(xml.value("model.management.abe.file"));
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

void ForestManagementEngine::prepareRun()
{
    mStandLayoutChanged = false; // can be changed by salvage operations / stand polygon changes
}

void ForestManagementEngine::finalizeRun()
{
    // empty the harvest counter; it will be filled again
    // during the (next) year.
    foreach (FMStand *stand, mStands)
        stand->resetHarvestCounter();

    //
    if (mStandLayoutChanged) {
        DebugTimer timer("ABE:stand_layout_update");
        // renew the internal stand grid
        FMStand **fm = mFMStandGrid.begin();
        for (int *p = standGrid()->grid().begin(); p!=standGrid()->grid().end(); ++p, ++fm)
            *fm = *p<0?0:mStandHash[*p];
        // renew neigborhood information in the stand grid
        const_cast<MapGrid*>(standGrid())->updateNeighborList();
        // renew the spatial indices
        const_cast<MapGrid*>(standGrid())->createIndex();
        mStandLayoutChanged = false;

        // now check the stands
        for (QVector<FMStand*>::iterator it=mStands.begin(); it!=mStands.end(); ++it)
            if (!(*it)->currentActivity())
                (*it)->initialize((*it)->stp());
    }
}

void ForestManagementEngine::setupOutputs()
{
    if (GlobalSettings::instance()->outputManager()->find("abeUnit"))
        return; // already set up
    GlobalSettings::instance()->outputManager()->addOutput(new UnitOut);
    GlobalSettings::instance()->outputManager()->addOutput(new ABEStandOut);
    //GlobalSettings::instance()->outputManager()->addOutput(new ABEStandRemovalOut);
}

AgentType *ForestManagementEngine::agentType(const QString &name)
{
    for (int i=0;i<mAgentTypes.count();++i)
        if (mAgentTypes[i]->name()==name)
            return mAgentTypes[i];
    return 0;
}


/*---------------------------------------------------------------------
 * multithreaded execution routines
---------------------------------------------------------------------*/

FMUnit *nc_execute_unit(FMUnit *unit)
{
    if (ForestManagementEngine::instance()->isCancel())
        return unit;

    //qDebug() << "called for unit" << unit;
    const QMultiMap<FMUnit*, FMStand*> &stand_map = ForestManagementEngine::instance()->stands();
    QMultiMap<FMUnit*, FMStand*>::const_iterator it = stand_map.constFind(unit);
    int executed = 0;
    int total = 0;
    while (it!=stand_map.constEnd() && it.key()==unit) {
        it.value()->stp()->executeRepeatingActivities(it.value());
        if (it.value()->execute())
            ++executed;
        if (ForestManagementEngine::instance()->isCancel())
            break;

        ++it;
        ++total;
    }
    if (ForestManagementEngine::instance()->isCancel())
        return unit;

    if (FMSTP::verbose())
        qCDebug(abe) << "execute unit'" << unit->id() << "', ran" << executed << "of" << total;

    // now run the scheduler
    unit->scheduler()->run();

    // collect the harvests
    it = stand_map.constFind(unit);
    while (it!=stand_map.constEnd() && it.key()==unit) {
        unit->addRealizedHarvest(it.value()->totalHarvest());
        ++it;
    }


    return unit;
}

FMUnit *nc_plan_update_unit(FMUnit *unit)
{
    if (ForestManagementEngine::instance()->isCancel())
        return unit;

    if (ForestManagementEngine::instance()->currentYear() % 10 == 0) {
        qCDebug(abe) << "*** execute decadal plan update ***";
        unit->managementPlanUpdate();
    }


    // first update happens *after* a full year of running ABE.
    if (ForestManagementEngine::instance()->currentYear()>1)
        unit->updatePlanOfCurrentYear();

    return unit;
}



void ForestManagementEngine::setup()
{
    QLoggingCategory::setFilterRules("abe.debug=true\n" \
                                     "abe.setup.debug=true"); // enable *all*

    DebugTimer time_setup("ABE:setup");
    clear();
    const XmlHelper &xml = GlobalSettings::instance()->settings();

    // (1) setup the scripting environment and load all the javascript code
    setupScripting();
    if (isCancel()) {
        throw IException(QString("ABE-Error (setup): %1").arg(mLastErrorMessage));
    }


    if (!GlobalSettings::instance()->model())
        throw IException("No model created.... invalid operation.");
    // (2) spatial data (stands, units, ...)
    const MapGrid *stand_grid = GlobalSettings::instance()->model()->standGrid();

    if (stand_grid==NULL || stand_grid->isValid()==false)
        throw IException("The ABE management model requires a valid stand grid.");

    QString data_file_name = GlobalSettings::instance()->path(xml.value("model.management.abe.agentDataFile"));
    CSVFile data_file(data_file_name);
    if (data_file.rowCount()==0)
        throw IException(QString("Stand-Initialization: the standDataFile file %1 is empty or missing!").arg(data_file_name));
    int ikey = data_file.columnIndex("id");
    int iunit = data_file.columnIndex("unit");
    int iagent = data_file.columnIndex("agent");
    int istp = data_file.columnIndex("stp");
    if (ikey<0 || iunit<0 || iagent<0)
        throw IException("setup ABE agentDataFile: one (or more) of the required columns 'id','unit', 'agent' not available.");

    QList<QString> unit_codes;
    QList<QString> agent_codes;
    QHash<FMStand*, QString> initial_stps;
    for (int i=0;i<data_file.rowCount();++i) {
        int stand_id = data_file.value(i,ikey).toInt();
        if (!stand_grid->isValid(stand_id))
            continue; // skip stands that are not in the map (e.g. when a smaller extent is simulated)
        if (FMSTP::verbose())
            qCDebug(abeSetup) << "setting up stand" << stand_id;

        // check agents
        QString agent_code = data_file.value(i, iagent).toString();
        Agent *agent=0;
        AgentType *at=0;
        if (!agent_codes.contains(agent_code)) {
            // create the agent / agent type
            at = agentType(agent_code);
            if (!at)
                throw IException(QString("Agent '%1' is not set up!").arg(agent_code));
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
        if (istp>-1) {
            QString stp = data_file.value(i, istp).toString();
            initial_stps[stand] = stp;
        }
        mMaxStandId = qMax(mMaxStandId, stand_id);

        mUnitStandMap.insertMulti(unit,stand);
        mStands.append(stand);

    }
    // set up the stand grid (visualizations)...
    // set up a hash for helping to establish stand-id <-> fmstand-link
    mStandHash.clear();
    for (int i=0;i<mStands.size(); ++i)
        mStandHash[mStands[i]->id()] = mStands[i];

    mFMStandGrid.setup(standGrid()->grid().metricRect(), standGrid()->grid().cellsize());
    mFMStandGrid.initialize(0);
    FMStand **fm = mFMStandGrid.begin();
    for (int *p = standGrid()->grid().begin(); p!=standGrid()->grid().end(); ++p, ++fm)
        *fm = *p<0?0:mStandHash[*p];

    mStandLayers.setGrid(mFMStandGrid);
    mStandLayers.clearClasses();
    mStandLayers.registerLayers();

    // now initialize STPs (if they are defined in the init file)
    for (QHash<FMStand*,QString>::iterator it=initial_stps.begin(); it!=initial_stps.end(); ++it) {
        FMStand *s = it.key();
        FMSTP* stp = s->unit()->agent()->type()->stpByName(it.value());
        if (stp) {
            s->initialize(stp);
        }
        if (isCancel()) {
            throw IException(QString("ABE-Error: init of stand %2: %1").arg(mLastErrorMessage).arg(s->id()));
        }
    }

    // now initialize the agents....
    foreach(AgentType *at, mAgentTypes) {
        at->setup();
        if (isCancel()) {
            throw IException(QString("ABE-Error: setup of agent '%2': %1").arg(mLastErrorMessage).arg(at->name()));
        }
    }

    // run the initial planning unit setup
    GlobalSettings::instance()->model()->threadExec().run(nc_plan_update_unit, mUnits);


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
    mCancel = false;
    mLastErrorMessage = QString();
}

void ForestManagementEngine::abortExecution(const QString &message)
{
    mLastErrorMessage = message;
    mCancel = true;
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

    prepareRun();

    {
    // launch the planning unit level update (annual and thorough analysis every ten years)
    DebugTimer plu("ABE:planUpdate");
    GlobalSettings::instance()->model()->threadExec().run(nc_plan_update_unit, mUnits);
    }

    GlobalSettings::instance()->model()->threadExec().run(nc_execute_unit, mUnits);
    if (isCancel()) {
        throw IException(QString("ABE-Error: %1").arg(mLastErrorMessage));
    }

    // create outputs
    GlobalSettings::instance()->outputManager()->execute("abeUnit");
    GlobalSettings::instance()->outputManager()->execute("abeStand");
    GlobalSettings::instance()->outputManager()->execute("abeStandRemoval");

    finalizeRun();

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

QStringList ForestManagementEngine::evaluateClick(const QPointF coord, const QString &grid_name)
{
    Q_UNUSED(grid_name); // for the moment
    // find the stand at coord.
    FMStand *stand = mFMStandGrid.constValueAt(coord);
    if (stand)
        return stand->info();
    return QStringList();
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

void ForestManagementEngine::addTreeRemoval(Tree *tree, int reason)
{
    // we use an 'int' instead of Tree:TreeRemovalType because it does not work
    // with forward declaration (and I dont want to include the tree.h header in this class header).
    FMStand *stand = mFMStandGrid.valueAt(tree->position());
    if (stand)
        stand->addTreeRemoval(tree, reason);
}

QMutex protect_split;
FMStand *ForestManagementEngine::splitExistingStand(FMStand *stand)
{
    // get a new stand-id
    // make sure that the Id is only used once.
    QMutexLocker protector(&protect_split);
    int new_stand_id = ++mMaxStandId;

    FMUnit *unit = const_cast<FMUnit*> (stand->unit());
    FMStand *new_stand = new FMStand(unit,new_stand_id);


    mUnitStandMap.insertMulti(unit,new_stand);
    mStands.append(new_stand);
    mStandHash[new_stand_id] = new_stand;

    mStandLayoutChanged = true;

    return new_stand;
}



} // namespace
