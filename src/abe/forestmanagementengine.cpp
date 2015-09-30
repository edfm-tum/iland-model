/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

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
#include "abestandremovalout.h"

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

    foreach (FMStand *stand, mStands) {
        stand->resetHarvestCounter();
    }

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
            if (!(*it)->currentActivity()) {
                (*it)->initialize();
            }
    }

}

void ForestManagementEngine::setupOutputs()
{
    if (GlobalSettings::instance()->outputManager()->find("abeUnit"))
        return; // already set up
    GlobalSettings::instance()->outputManager()->addOutput(new UnitOut);
    GlobalSettings::instance()->outputManager()->addOutput(new ABEStandOut);
    GlobalSettings::instance()->outputManager()->addOutput(new ABEStandRemovalOut);
}

void ForestManagementEngine::runJavascript()
{
    scriptBridge()->setExecutionContext(0, false);
    QJSValue handler = scriptEngine()->globalObject().property("run");
    if (handler.isCallable()) {
        QJSValue result = handler.call(QJSValueList() << mCurrentYear);
        if (FMSTP::verbose())
            qCDebug(abe) << "executing 'run' function for year" << mCurrentYear << ", result:" << result.toString();
    }

    handler = scriptEngine()->globalObject().property("runStand");
    if (handler.isCallable()) {
        qCDebug(abe) << "running the 'runStand' javascript function for" << mStands.size() << "stands.";
        foreach (FMStand *stand, mStands) {
            scriptBridge()->setExecutionContext(stand, true);
            handler.call(QJSValueList() << mCurrentYear);
        }
    }
}

AgentType *ForestManagementEngine::agentType(const QString &name)
{
    for (int i=0;i<mAgentTypes.count();++i)
        if (mAgentTypes[i]->name()==name)
            return mAgentTypes[i];
    return 0;
}

Agent *ForestManagementEngine::agent(const QString &name)
{
    for (int i=0;i<mAgents.count();++i)
        if (mAgents[i]->name()==name)
            return mAgents[i];
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
        //MapGrid::freeLocksForStand( it.value()->id() );
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
        unit->runAgent();
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

    DebugTimer time_setup("ABE:setupScripting");
    clear();

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

    const XmlHelper &xml = GlobalSettings::instance()->settings();

    QString data_file_name = GlobalSettings::instance()->path(xml.value("model.management.abe.agentDataFile"));
    qCDebug(abeSetup) << "loading ABE agentDataFile" << data_file_name << "...";
    CSVFile data_file(data_file_name);
    if (data_file.rowCount()==0)
        throw IException(QString("Stand-Initialization: the standDataFile file %1 is empty or missing!").arg(data_file_name));
    int ikey = data_file.columnIndex("id");
    int iunit = data_file.columnIndex("unit");
    int iagent = data_file.columnIndex("agent");
    int iagent_type = data_file.columnIndex("agentType");
    int istp = data_file.columnIndex("stp");
    // unit properties
    int ispeciescomp = data_file.columnIndex("speciesComposition");
    int ithinning = data_file.columnIndex("thinningIntensity");
    int irotation = data_file.columnIndex("U");
    int iMAI = data_file.columnIndex("MAI");
    int iharvest_mode = data_file.columnIndex("harvestMode");


    if (ikey<0 || iunit<0)
        throw IException("setup ABE agentDataFile: one (or two) of the required columns 'id' or 'unit' not available.");
    if (iagent<0 && iagent_type<0)
        throw IException("setup ABE agentDataFile: the columns 'agent' or 'agentType' are not available. You have to include at least one of the columns.");


    QList<QString> unit_codes;
    QHash<FMStand*, QString> initial_stps;
    for (int i=0;i<data_file.rowCount();++i) {
        int stand_id = data_file.value(i,ikey).toInt();
        if (!stand_grid->isValid(stand_id))
            continue; // skip stands that are not in the map (e.g. when a smaller extent is simulated)
        if (FMSTP::verbose())
            qCDebug(abeSetup) << "setting up stand" << stand_id;

        // check agents
        QString agent_code = iagent>-1 ? data_file.value(i, iagent).toString() : QString();
        QString agent_type_code = iagent_type>-1 ? data_file.value(i, iagent_type).toString() : QString();
        QString unit_id = data_file.value(i, iunit).toString();

        Agent *ag=0;
        AgentType *at=0;
        if (agent_code.isEmpty() && agent_type_code.isEmpty())
            throw IException(QString("setup ABE agentDataFile row '%1': no code for columns 'agent' and 'agentType' available.").arg(i) );

        if (!agent_code.isEmpty()) {
            // search for a specific agent
            ag = agent(agent_code);
            if (!ag)
                throw IException(QString("Agent '%1' is not set up (row '%2')! Use the 'newAgent()' JS function of agent-types to add agent definitions.").arg(agent_code).arg(i));
            at = ag->type();

        } else {
            // look up the agent type and create the agent on the fly
            // create the agent / agent type
            at = agentType(agent_type_code);
            if (!at)
                throw IException(QString("Agent type '%1' is not set up (row '%2')! Use the 'addAgentType()' JS function to add agent-type definitions.").arg(agent_type_code).arg(i));

            if (!unit_codes.contains(unit_id)) {
                // we create an agent for the unit only once (per unit)
                ag = at->createAgent();
            }
        }


        // check units
        FMUnit *unit = 0;
        if (!unit_codes.contains(unit_id)) {
            // create the unit
            unit = new FMUnit(ag);
            unit->setId(unit_id);
            if (iharvest_mode>-1)
                unit->setHarvestMode( data_file.value(i, iharvest_mode).toString());
            if (ithinning>-1)
                unit->setThinningIntensity( data_file.value(i, ithinning).toInt() );
            if (irotation>-1)
                unit->setU( data_file.value(i, irotation).toDouble() );
            if (iMAI)
                unit->setAverageMAI(data_file.value(i, iMAI).toDouble());
            if (ispeciescomp>-1) {
                int index;
                index = at->speciesCompositionIndex( data_file.value(i, ispeciescomp).toString() );
                if (index==-1)
                    throw IException(QString("The species composition '%1' for unit '%2' is not a valid composition type (agent type: '%3').").arg(data_file.value(i, ispeciescomp).toString()).arg(unit->id()).arg(at->name()));
                unit->setTargetSpeciesCompositionIndex( index );
            }
            mUnits.append(unit);
            unit_codes.append(unit_id);
            ag->addUnit(unit); // add the unit to the list of managed units of the agent
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

    // count the number of stands within each unit
    foreach(FMUnit *unit, mUnits)
        unit->setNumberOfStands( mUnitStandMap.count(unit) );

    // set up the stand grid (visualizations)...
    // set up a hash for helping to establish stand-id <-> fmstand-link
    mStandHash.clear();
    for (int i=0;i<mStands.size(); ++i) {
        mStandHash[mStands[i]->id()] = mStands[i];
    }

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
            s->setSTP(stp);
        } else {
            qCDebug(abeSetup) << "Warning during reading of CSV setup file: the STP '" << it.value() << "' is not valid for Agenttype: " << s->unit()->agent()->type()->name();
        }
    }
    qCDebug(abeSetup) << "ABE setup completed.";
}

void ForestManagementEngine::initialize()
{

    DebugTimer time_setup("ABE:setup");

    foreach (FMStand* stand, mStands) {
        if (stand->stp()) {

            stand->setU( stand->unit()->U() );
            stand->setThinningIntensity( stand->unit()->thinningIntensity() );
            stand->setTargetSpeciesIndex( stand->unit()->targetSpeciesIndex() );

            stand->initialize();
            if (isCancel()) {
                throw IException(QString("ABE-Error: init of stand %2: %1").arg(mLastErrorMessage).arg(stand->id()));
            }
        }
    }

    // now initialize the agents....
    foreach(Agent *ag, mAgents) {
        ag->setup();
        if (isCancel()) {
            throw IException(QString("ABE-Error: setup of agent '%2': %1").arg(mLastErrorMessage).arg(ag->name()));
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

void ForestManagementEngine::runOnInit()
{
    if (GlobalSettings::instance()->scriptEngine()->globalObject().hasProperty("onInit")) {
        QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate("onInit()");
        if (result.isError())
            qCDebug(abeSetup) << "Javascript Error in global 'onInit'-Handler:" << result.toString();

    }
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

    // execute an event handler before invoking the ABE core
    runJavascript();

    {
    // launch the planning unit level update (annual and thorough analysis every ten years)
    DebugTimer plu("ABE:planUpdate");
    GlobalSettings::instance()->model()->threadExec().run(nc_plan_update_unit, mUnits, true);
    }

    GlobalSettings::instance()->model()->threadExec().run(nc_execute_unit, mUnits, true); // force single thread operation for now
    if (isCancel()) {
        throw IException(QString("ABE-Error: %1").arg(mLastErrorMessage));
    }

    // create outputs
    {
    DebugTimer plu("ABE:outputs");
    GlobalSettings::instance()->outputManager()->execute("abeUnit");
    GlobalSettings::instance()->outputManager()->execute("abeStand");
    GlobalSettings::instance()->outputManager()->execute("abeStandRemoval");
    }

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
    if (mStandHash.contains(stand_id))
        return mStandHash[stand_id];

    // exhaustive search... should not happen
    qCDebug(abe) << "ForestManagementEngine::stand() fallback to exhaustive search.";
    for (QVector<FMStand*>::const_iterator it=mStands.constBegin(); it!=mStands.constEnd(); ++it)
        if ( (*it)->id() == stand_id)
            return *it;
    return 0;
}

QStringList ForestManagementEngine::standIds() const
{
    QStringList standids;
    foreach(FMStand *s, mStands)
        standids.push_back(QString::number(s->id()));
    return standids;
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

    unit->setNumberOfStands( mUnitStandMap.count(unit) );

    mStandLayoutChanged = true;

    return new_stand;
}



} // namespace
