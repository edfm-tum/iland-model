/** @class Model
  Main object of the iLand model composited of various sub models / sub components.
  */
#include "global.h"
#include "model.h"
#include "sqlhelper.h"

#include "xmlhelper.h"
#include "environment.h"
#include "timeevents.h"
#include "helper.h"
#include "resourceunit.h"
#include "climate.h"
#include "speciesset.h"
#include "standloader.h"
#include "tree.h"
#include "management.h"
#include "modelsettings.h"
#include "standstatistics.h"
#include "mapgrid.h"
#include "modelcontroller.h"
#include "modules.h"
#include "dem.h"

#include "outputmanager.h"

#include <QtCore>
#include <QtXml>

/** iterate over all trees of the model. return NULL if all trees processed.
  Usage:
  @code
  AllTreeIterator trees(model);
  while (Tree *tree = trees.next()) { // returns NULL when finished.
     tree->something(); // do something
  }
  @endcode  */
Tree *AllTreeIterator::next()
{

    if (!mTreeEnd) {
        // initialize to first ressource unit
        mRUIterator = mModel->ruList().constBegin();
        // fast forward to the first RU with trees
        while (mRUIterator!=mModel->ruList().constEnd()) {
            if ((*mRUIterator)->trees().count()>0)
                break;
            mRUIterator++;
        }
            // finished if all RU processed
        if (mRUIterator == mModel->ruList().constEnd())
            return NULL;
        mTreeEnd = &((*mRUIterator)->trees().back()) + 1; // let end point to "1 after end" (STL-style)
        mCurrent = &((*mRUIterator)->trees().front());
    }
    if (mCurrent==mTreeEnd) {
        mRUIterator++; // switch to next RU (loop until RU with trees is found)
        while (mRUIterator!=mModel->ruList().constEnd()) {
            if ((*mRUIterator)->trees().count()>0) {
                break;
            }
            mRUIterator++;
        }
        if (mRUIterator == mModel->ruList().constEnd()) {
            mCurrent = NULL;
            return NULL; // finished!!
        }else {
            mTreeEnd = &((*mRUIterator)->trees().back()) + 1;
            mCurrent = &((*mRUIterator)->trees().front());
        }
    }

    return mCurrent++;
}
Tree *AllTreeIterator::nextLiving()
{
    while (Tree *t = next())
        if (!t->isDead()) return t;
    return NULL;
}
Tree *AllTreeIterator::current() const
{
    return mCurrent?mCurrent-1:NULL;
}


ModelSettings Model::mSettings;
Model::Model()
{
    initialize();
    GlobalSettings::instance()->setModel(this);
    QString dbg="running in release mode.";
    DBGMODE( dbg="running in debug mode."; );
    qDebug() << dbg;
}

Model::~Model()
{
    clear();
    GlobalSettings::instance()->setModel(NULL);
}

/** Initial setup of the Model.
  */
void Model::initialize()
{
   mSetup = false;
   GlobalSettings::instance()->setCurrentYear(0);
   mGrid = 0;
   mHeightGrid = 0;
   mManagement = 0;
   mEnvironment = 0;
   mTimeEvents = 0;
   mStandGrid = 0;
   mModules = 0;
   mDEM = 0;
}

/** sets up the simulation space.
*/
void Model::setupSpace()
{
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.world"));
    double cellSize = xml.value("cellSize", "2").toDouble();
    double width = xml.value("width", "100").toDouble();
    double height = xml.value("height", "100").toDouble();
    double buffer = xml.value("buffer", "60").toDouble();
    mModelRect = QRectF(0., 0., width, height);

    qDebug() << QString("setup of the world: %1x%2m with cell-size=%3m and %4m buffer").arg(width).arg(height).arg(cellSize).arg(buffer);

    QRectF total_grid(QPointF(-buffer, -buffer), QPointF(width+buffer, height+buffer));
    qDebug() << "setup grid rectangle:" << total_grid;

    if (mGrid)
        delete mGrid;
    mGrid = new FloatGrid(total_grid, cellSize);
    mGrid->initialize(1.f);
    if (mHeightGrid)
        delete mHeightGrid;
    mHeightGrid = new HeightGrid(total_grid, cellSize*5);
    mHeightGrid->wipe(); // set all to zero
    Tree::setGrid(mGrid, mHeightGrid);

    // setup the spatial location of the project area
    if (xml.hasNode("location")) {
        // setup of spatial location
        double loc_x = xml.valueDouble("location.x");
        double loc_y = xml.valueDouble("location.y");
        double loc_z = xml.valueDouble("location.z");
        double loc_rot = xml.valueDouble("location.rotation");
        setupGISTransformation(loc_x, loc_y, loc_z, loc_rot);
        qDebug() << "setup of spatial location: x/y/z" << loc_x << loc_y << loc_z << "rotation:" << loc_rot;
    } else {
        setupGISTransformation(0., 0., 0., 0.);
    }

    // load environment (multiple climates, speciesSets, ...
    if (mEnvironment)
        delete mEnvironment;
    mEnvironment = new Environment();

    if (xml.valueBool("environmentEnabled", false)) {
        QString env_file = GlobalSettings::instance()->path(xml.value("environmentFile"));
        bool grid_mode = (xml.value("environmentMode")=="grid");
        QString grid_file = GlobalSettings::instance()->path(xml.value("environmentGrid"));
        if (grid_mode && QFile::exists(grid_file))
            mEnvironment->setGridMode(grid_file);

        if (!mEnvironment->loadFromFile(env_file))
            return;
        // retrieve species sets and climates:
        mSpeciesSets << mEnvironment->speciesSetList();
        mClimates << mEnvironment->climateList();
    } else {
        // load and prepare default values
        // (2) SpeciesSets: currently only one a global species set.
        SpeciesSet *speciesSet = new SpeciesSet();
        mSpeciesSets.push_back(speciesSet);
        speciesSet->setup();
        // Climate...
        Climate *c = new Climate();
        mClimates.push_back(c);
        mEnvironment->setDefaultValues(c, speciesSet);
    } // environment?

    // time series data
    if (xml.valueBool(".timeEventsEnabled", false)) {
        mTimeEvents = new TimeEvents();
        mTimeEvents->loadFromFile(GlobalSettings::instance()->path(xml.value("timeEventsFile"), "script"));
    }


    // simple case: create ressource units in a regular grid.
    mRUmap.clear();
    if (xml.valueBool("resourceUnitsAsGrid")) {
        mRUmap.setup(QRectF(0., 0., width, height),100.); // Grid, that holds positions of resource units
        ResourceUnit **p; // ptr to ptr!
        ResourceUnit *new_ru;

        int ru_index = 0;
        for (p=mRUmap.begin(); p!=mRUmap.end(); ++p) {
            QRectF r = mRUmap.cellRect(mRUmap.indexOf(p));
            mEnvironment->setPosition( r.center() ); // if environment is 'disabled' default values from the project file are used.
            new_ru = new ResourceUnit(ru_index++); // create resource unit
            new_ru->setClimate( mEnvironment->climate() );
            new_ru->setSpeciesSet( mEnvironment->speciesSet() );
            new_ru->setup();
            new_ru->setID( mEnvironment->currentID() ); // set id of resource unit in grid mode
            new_ru->setBoundingBox(r);
            mRU.append(new_ru);
            *p = new_ru; // save also in the RUmap grid
        }

        // now store the pointers in the grid.
        // Important: This has to be done after the mRU-QList is complete - otherwise pointers would
        // point to invalid memory when QList's memory is reorganized (expanding)
        ru_index = 0;
        for (p=mRUmap.begin();p!=mRUmap.end(); ++p) {
            *p = mRU.value(ru_index++);
        }
        qDebug() << "created a grid of ResourceUnits: count=" << mRU.count();

        bool mask_is_setup = false;

        if (xml.valueBool("standGrid.enabled")) {
            QString fileName = GlobalSettings::instance()->path(xml.value("standGrid.fileName"));
            mStandGrid = new MapGrid(fileName);

            if (mStandGrid->isValid()) {
                for (int i=0;i<mStandGrid->grid().count();i++)
                    mHeightGrid->valueAtIndex(i).setValid( mStandGrid->grid().constValueAtIndex(i)!=-1 );
            }
            mask_is_setup = true;
        }

        calculateStockableArea();

        // setup of the project area mask
        if (!mask_is_setup && xml.valueBool("areaMask.enabled", false) && xml.hasNode("areaMask.imageFile")) {
            // to be extended!!! e.g. to load ESRI-style text files....
            // setup a grid with the same size as the height grid...
            FloatGrid tempgrid((int)mHeightGrid->cellsize(), mHeightGrid->sizeX(), mHeightGrid->sizeY());
            QString fileName = GlobalSettings::instance()->path(xml.value("areaMask.imageFile"));
            loadGridFromImage(fileName, tempgrid); // fetch from image
            for (int i=0;i<tempgrid.count(); i++)
                mHeightGrid->valueAtIndex(i).setValid( tempgrid.valueAtIndex(i)>0.99 );
            qDebug() << "loaded project area mask from" << fileName;
        }

        // list of "valid" resource units
        QList<ResourceUnit*> valid_rus;
        foreach(ResourceUnit* ru, mRU)
            if (ru->id()!=-1)
                valid_rus.append(ru);

        // setup of the digital elevation map (if present)
        QString dem_file = xml.value("DEM");
        if (!dem_file.isEmpty()) {
            mDEM = new DEM(GlobalSettings::instance()->path(dem_file));
            // add them to the visuals...
            GlobalSettings::instance()->controller()->addGrid(mDEM, "DEM height", GridViewRainbow, 0, 1000);
            GlobalSettings::instance()->controller()->addGrid(mDEM->slopeGrid(), "DEM slope", GridViewRainbow, 0, 3);
            GlobalSettings::instance()->controller()->addGrid(mDEM->aspectGrid(), "DEM aspect", GridViewRainbow, 0, 360);
            GlobalSettings::instance()->controller()->addGrid(mDEM->viewGrid(), "DEM view", GridViewGray, 0, 1);

        }

        // setup of external modules
        mModules->setup();
        if (mModules->hasSetupResourceUnits()) {
            for (ResourceUnit **p=mRUmap.begin(); p!=mRUmap.end(); ++p) {
                QRectF r = mRUmap.cellRect(mRUmap.indexOf(p));
                mEnvironment->setPosition( r.center() ); // if environment is 'disabled' default values from the project file are used.
                mModules->setupResourceUnit( *p );
            }
        }

        // setup the helper that does the multithreading
        threadRunner.setup(valid_rus);
        threadRunner.setMultithreading(GlobalSettings::instance()->settings().valueBool("system.settings.multithreading"));
        threadRunner.print();

    } else  {
        throw IException("resourceUnitsAsGrid MUST be set to true - at least currently :)");
    }
    mSetup = true;
}


/** clear() frees all ressources allocated with the run of a simulation.

  */
void Model::clear()
{
    mSetup = false;
    qDebug() << "Model clear: attempting to clear" << mRU.count() << "RU, " << mSpeciesSets.count() << "SpeciesSets.";
    // clear ressource units
    qDeleteAll(mRU); // delete ressource units (and trees)
    mRU.clear();

    qDeleteAll(mSpeciesSets); // delete species sets
    mSpeciesSets.clear();

    // delete climate data
    qDeleteAll(mClimates);

    // delete the grids
    if (mGrid)
        delete mGrid;
    if (mHeightGrid)
        delete mHeightGrid;
    if (mManagement)
        delete mManagement;
    if (mEnvironment)
        delete mEnvironment;
    if (mTimeEvents)
        delete mTimeEvents;
    if (mStandGrid)
        delete mStandGrid;
    if (mModules)
        delete mModules;
    if (mDEM)
        delete mDEM;

    mGrid = 0;
    mHeightGrid = 0;
    mManagement = 0;
    mEnvironment = 0;
    mTimeEvents = 0;
    mStandGrid  = 0;
    mModules = 0;
    mDEM = 0;

    GlobalSettings::instance()->outputManager()->close();

    qDebug() << "Model ressources freed.";
}

/** Setup of the Simulation.
  This really creates the simulation environment and does the setup of various aspects.
  */
void Model::loadProject()
{
    DebugTimer dt("load project");
    GlobalSettings *g = GlobalSettings::instance();
    const XmlHelper &xml = g->settings();

    g->clearDatabaseConnections();
    // database connections: reset
    GlobalSettings::instance()->clearDatabaseConnections();
    // input and climate connection
    // see initOutputDatabase() for output database
    QString dbPath = g->path( xml.value("system.database.in"), "database");
    GlobalSettings::instance()->setupDatabaseConnection("in", dbPath, true);
    dbPath = g->path( xml.value("system.database.climate"), "database");
    GlobalSettings::instance()->setupDatabaseConnection("climate", dbPath, true);

    mSettings.loadModelSettings();
    mSettings.print();
    // random seed: if stored value is <> 0, use this as the random seed (and produce hence always an equal sequence of random numbers)
    uint seed = xml.value("system.settings.randomSeed","0").toUInt();
    if (seed!=0)
        mtRand().seed(seed);
    else
        mtRand().seed(); // seed with a random value
    // linearization of expressions: if true *and* linearize() is explicitely called, then
    // function results will be cached over a defined range of values.
    bool do_linearization = xml.valueBool("system.settings.expressionLinearizationEnabled", false);
    Expression::setLinearizationEnabled(do_linearization);
    if (do_linearization)
        qDebug() << "The linearization of certains expressions is enabled (performance optimization).";

    // log level
    QString log_level = xml.value("system.settings.logLevel", "debug").toLower();
    if (log_level=="debug") setLogLevel(0);
    if (log_level=="info") setLogLevel(1);
    if (log_level=="warning") setLogLevel(2);
    if (log_level=="error") setLogLevel(3);

    // snag dynamics / soil model enabled? (info used during setup of world)
    changeSettings().carbonCycleEnabled = xml.valueBool("model.settings.carbonCycleEnabled", false);
    // class size of snag classes
    Snag::setupThresholds(xml.valueDouble("model.settings.soil.swdDBHClass12"),
                          xml.valueDouble("model.settings.soil.swdDBHClass23"));

    // setup of modules
    if (mModules)
        delete mModules;
    mModules = new Modules();

    setupSpace();
    if (mRU.isEmpty())
        throw IException("Setup of Model: no resource units present!");

    // (3) additional issues
    // (3.1) setup of regeneration
    changeSettings().regenerationEnabled = xml.valueBool("model.settings.regenerationEnabled", false);
    if (settings().regenerationEnabled) {
        foreach(SpeciesSet *ss, mSpeciesSets)
            ss->setupRegeneration();
    }

    Sapling::setRecruitmentVariation(xml.valueDouble("model.settings.seedDispersal.recruitmentDimensionVariation",0.1));

    // (3.2) management
    QString mgmtFile = xml.value("model.management.file");
    if (!mgmtFile.isEmpty() && xml.valueBool("model.management.enabled")) {
        mManagement = new Management();
        QString path = GlobalSettings::instance()->path(mgmtFile, "script");
        mManagement->loadScript(path);
        qDebug() << "setup management using script" << path;
    }


}


ResourceUnit *Model::ru(QPointF coord)
{
    if (!mRUmap.isEmpty() && mRUmap.coordValid(coord))
        return mRUmap.valueAt(coord);
    return ru(); // default RU if there is only one
}

void Model::initOutputDatabase()
{
    GlobalSettings *g = GlobalSettings::instance();
    QString dbPath = g->path(g->settings().value("system.database.out"), "output");
    // create run-metadata
    int maxid = SqlHelper::queryValue("select max(id) from runs", g->dbin()).toInt();

    maxid++;
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    SqlHelper::executeSql(QString("insert into runs (id, timestamp) values (%1, '%2')").arg(maxid).arg(timestamp), g->dbin());
    // replace path information
    dbPath.replace("$id$", QString::number(maxid));
    dbPath.replace("$date$", timestamp);
    // setup final path
   g->setupDatabaseConnection("out", dbPath, false);

}

ResourceUnit *nc_sapling_growth(ResourceUnit *unit)
{
    unit->setRandomGenerator();
    try {
        // define a height map for the current resource unit on the stack and clear it
        float sapling_map[cPxPerRU*cPxPerRU];
        for (int i=0;i<cPxPerRU*cPxPerRU;i++) sapling_map[i]=0.f;
        unit->setSaplingHeightMap(sapling_map);


        // (1) calculate the growth of (already established) saplings (populate sapling map)
        foreach (const ResourceUnitSpecies *rus, unit->ruSpecies()) {
            const_cast<ResourceUnitSpecies*>(rus)->calclulateSaplingGrowth();
        }

    } catch (const IException& e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }
    return unit;

}

/// multithreaded running function for the resource unit level establishment
ResourceUnit *nc_establishment(ResourceUnit *unit)
{
    unit->setRandomGenerator();

    try {

        // (2) calculate the establishment probabilities of new saplings
        foreach (const ResourceUnitSpecies *rus, unit->ruSpecies()) {
            const_cast<ResourceUnitSpecies*>(rus)->calclulateEstablishment();
        }

    } catch (const IException& e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }

    return unit;
}

/// multithreaded execution of the carbon cycle routine
ResourceUnit *nc_carbonCycle(ResourceUnit *unit)
{
    try {
        // (1) do calculations on snag dynamics for the resource unit
        unit->calculateCarbonCycle();
        // (2) do the soil carbon and nitrogen dynamics calculations (ICBM/2N)
    } catch (const IException& e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }

    return unit;
}

/// beforeRun performs several steps before the models starts running.
/// inter alia: * setup of the stands
///             * setup of the climates
void Model::beforeRun()
{
    // setup outputs
    // setup output database
    if (GlobalSettings::instance()->dbout().isOpen())
        GlobalSettings::instance()->dbout().close();
    initOutputDatabase();
    GlobalSettings::instance()->outputManager()->setup();
    GlobalSettings::instance()->clearDebugLists();

    // initialize stands
    {
    DebugTimer loadtrees("load trees");
    StandLoader loader(this);
    loader.processInit();
    }

    // load climate
    {
    DebugTimer loadclim("load climate");
    foreach(Climate *c, mClimates)
        if (!c->isSetup())
            c->setup();

    }

    { DebugTimer loadinit("load standstatistics");
    Tree::setGrid(mGrid, mHeightGrid);
    applyPattern();
    readPattern();

    // force the compilation of initial stand statistics
    createStandStatistics();
    }

    // outputs to create with inital state (without any growth) are called here:
    GlobalSettings::instance()->outputManager()->execute("stand"); // year=0
    GlobalSettings::instance()->outputManager()->execute("sapling"); // year=0
    GlobalSettings::instance()->outputManager()->execute("tree"); // year=0
    GlobalSettings::instance()->outputManager()->execute("dynamicstand"); // year=0

    GlobalSettings::instance()->setCurrentYear(1); // set to first year
}

/** Main model runner.
  The sequence of actions is as follows:
  (1) Load the climate of the new year
  (2) Reset statistics for resource unit as well as for dead/managed trees
  (3) Invoke Management. Also the dedicated spot for the actions of other disturbances.
  (4) *after* that, calculate Light patterns
  (5) 3PG on stand level, tree growth. Clear stand-statistcs before they are filled by single-tree-growth. calculate water cycle (with LAIs before management)
  (6) execute Regeneration
  (7) calculate statistics for the year
  (8) write database outputs
  */
void Model::runYear()
{
    DebugTimer t("Model::runYear()");
    GlobalSettings::instance()->systemStatistics()->reset();
    // initalization at start of year for external modules
    mModules->yearBegin();
    // execute scheduled events for the current year
    if (mTimeEvents)
        mTimeEvents->run();

    // load the next year of the climate database
    foreach(Climate *c, mClimates)
        c->nextYear();

    // reset statistics
    foreach(ResourceUnit *ru, mRU)
        ru->newYear();

    foreach(SpeciesSet *set, mSpeciesSets)
        set->newYear();
    // management
    if (mManagement) {
        DebugTimer t("management");
        mManagement->run();
        GlobalSettings::instance()->systemStatistics()->tManagement+=t.elapsed();
    }

    // process a cycle of individual growth
    applyPattern(); // create Light Influence Patterns
    readPattern(); // readout light state of individual trees
    grow(); // let the trees grow (growth on stand-level, tree-level, mortality)

    // regeneration
    if (settings().regenerationEnabled) {
        // seed dispersal
        DebugTimer tseed("Regeneration and Establishment");
        foreach(SpeciesSet *set, mSpeciesSets)
            set->regeneration(); // parallel execution for each species set

        GlobalSettings::instance()->systemStatistics()->tSeedDistribution+=tseed.elapsed();
        tseed.showElapsed();
        // establishment
        { DebugTimer t("saplingGrowth");
        executePerResourceUnit( nc_sapling_growth, false /* true: force single thraeded operation */);
        GlobalSettings::instance()->systemStatistics()->tSaplingGrowth+=t.elapsed();
        }
        {
        DebugTimer t("establishment");
        executePerResourceUnit( nc_establishment, false /* true: force single thraeded operation */);
        GlobalSettings::instance()->systemStatistics()->tEstablishment+=t.elapsed();
        }
    }

    // calculate soil / snag dynamics
    if (settings().carbonCycleEnabled) {
        DebugTimer ccycle("carbon cylce");
        executePerResourceUnit( nc_carbonCycle, false /* true: force single thraeded operation */);
        GlobalSettings::instance()->systemStatistics()->tCarbonCycle+=ccycle.elapsed();

    }

    // external modules/disturbances
    mModules->run();

    foreach(ResourceUnit *ru, mRU) {
        // remove trees that died during disturbances. Note that cleanTreeList() is cheap if we have no dead trees
        ru->cleanTreeList();
    }

    DebugTimer toutput("outputs");
    // calculate statistics
    foreach(ResourceUnit *ru, mRU)
        ru->yearEnd();

    // create outputs
    OutputManager *om = GlobalSettings::instance()->outputManager();
    om->execute("tree"); // single tree output
    om->execute("stand"); //resource unit level x species
    om->execute("sapling"); // sapling layer per RU x species
    om->execute("production_month"); // 3pg responses growth per species x RU x month
    om->execute("dynamicstand"); // output with user-defined columns (based on species x RU)
    om->execute("standdead"); // resource unit level x species
    om->execute("management"); // resource unit level x species
    om->execute("carbon"); // resource unit level, carbon pools above and belowground
    om->execute("carbonflow"); // resource unit level, GPP, NPP and total carbon flows (atmosphere, harvest, ...)

    GlobalSettings::instance()->systemStatistics()->tWriteOutput+=toutput.elapsed();
    GlobalSettings::instance()->systemStatistics()->tTotalYear+=t.elapsed();
    GlobalSettings::instance()->systemStatistics()->writeOutput();

    GlobalSettings::instance()->setCurrentYear(GlobalSettings::instance()->currentYear()+1);
}



void Model::afterStop()
{
    // do some cleanup
}

/// multithreaded running function for LIP printing
ResourceUnit* nc_applyPattern(ResourceUnit *unit)
{

    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator tend = unit->trees().end();
    unit->setRandomGenerator();

    try {

        // light concurrence influence
        if (!GlobalSettings::instance()->settings().paramValueBool("torus")) {
            // height dominance grid
            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).heightGrid(); // just do it ;)

            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).applyLIP(); // just do it ;)

        } else {
            // height dominance grid
            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).heightGrid_torus(); // just do it ;)

            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).applyLIP_torus(); // do it the wraparound way
        }
        return unit;
    } catch (const IException &e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }
    return unit;
}

/// multithreaded running function for LIP value extraction
ResourceUnit *nc_readPattern(ResourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();
    try {
        if (!GlobalSettings::instance()->settings().paramValueBool("torus")) {
            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).readLIF(); // multipliactive approach
        } else {
            for (tit=unit->trees().begin(); tit!=tend; ++tit)
                (*tit).readLIF_torus(); // do it the wraparound way
        }
    } catch (const IException &e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }
    return unit;
}

/// multithreaded running function for the growth of individual trees
ResourceUnit *nc_grow(ResourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();
    unit->setRandomGenerator();
    try {
        unit->beforeGrow(); // reset statistics
        // calculate light responses
        // responses are based on *modified* values for LightResourceIndex
        for (tit=unit->trees().begin(); tit!=tend; ++tit) {
            (*tit).calcLightResponse();
        }

        unit->calculateInterceptedArea();

        for (tit=unit->trees().begin(); tit!=tend; ++tit) {
            (*tit).grow(); // actual growth of individual trees
        }
    } catch (const IException &e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }

    GlobalSettings::instance()->systemStatistics()->treeCount+=unit->trees().count();
    return unit;
}

/// multithreaded running function for the resource level production
ResourceUnit *nc_production(ResourceUnit *unit)
{
    try {
        unit->setRandomGenerator();
        unit->production();
    } catch (const IException &e) {
        GlobalSettings::instance()->controller()->throwError(e.message());
    }
    return unit;
}


void Model::test()
{
    // Test-funktion: braucht 1/3 time von readGrid()
    DebugTimer t("test");
    FloatGrid averaged = mGrid->averaged(10);
    int count = 0;
    float *end = averaged.end();
    for (float *p=averaged.begin(); p!=end; ++p)
        if (*p > 0.9)
            count++;
    qDebug() << count << "LIF>0.9 of " << averaged.count();
}

void Model::applyPattern()
{

    DebugTimer t("applyPattern()");
    // intialize grids...
    mGrid->initialize(1.f);

    // initialize height grid with a value of 4m. This is the height of the regeneration layer
    for (HeightGridValue *h=mHeightGrid->begin();h!=mHeightGrid->end();++h) {
        h->resetCount(); // set count = 0, but do not touch the flags
        h->height = 4.f;
    }

    threadRunner.run(nc_applyPattern);
    GlobalSettings::instance()->systemStatistics()->tApplyPattern+=t.elapsed();
}

void Model::readPattern()
{
    DebugTimer t("readPattern()");
    threadRunner.run(nc_readPattern);
    GlobalSettings::instance()->systemStatistics()->tReadPattern+=t.elapsed();

}

/** Main function for the growth of stands and trees.
   This includes several steps.
   (1) calculate the stocked area (i.e. count pixels in height grid)
   (2) 3PG production (including response calculation, water cycle)
   (3) single tree growth (including mortality)
   (4) cleanup of tree lists (remove dead trees)
  */
void Model::grow()
{

    if (!settings().growthEnabled)
        return;
    { DebugTimer t("growRU()");
    calculateStockedArea();

    // multithreaded: mutex for the message handler in mainwindow solved the crashes.
    threadRunner.run(nc_production);
    }

    DebugTimer t("growTrees()");
    threadRunner.run(nc_grow); // actual growth of individual trees

    foreach(ResourceUnit *ru, mRU) {
       ru->cleanTreeList();
       ru->afterGrow();
       //qDebug() << (b-n) << "trees died (of" << b << ").";
   }
   GlobalSettings::instance()->systemStatistics()->tTreeGrowth+=t.elapsed();
}

/** calculate for each resource unit the fraction of area which is stocked.
  This is done by checking the pixels of the global height grid.
  */
void Model::calculateStockedArea()
{
    // iterate over the whole heightgrid and count pixels for each ressource unit
    HeightGridValue *end = mHeightGrid->end();
    QPointF cp;
    ResourceUnit *ru;
    for (HeightGridValue *i=mHeightGrid->begin(); i!=end; ++i) {
        cp = mHeightGrid->cellCenterPoint(mHeightGrid->indexOf(i));
        if (mRUmap.coordValid(cp)) {
            ru = mRUmap.valueAt(cp);
            if (ru) {
                ru->countStockedPixel( (*i).count()>0 );
            }
        }

    }
}

/** calculate for each resource unit the stockable area.
  "stockability" is determined by the isValid flag of resource units which in turn
  is derived from stand grid values.
  */
void Model::calculateStockableArea()
{

    foreach(ResourceUnit *ru, mRU) {
// //
//        if (ru->id()==-1) {
//            ru->setStockableArea(0.);
//            continue;
//        }
        GridRunner<HeightGridValue> runner(*mHeightGrid, ru->boundingBox());
        int valid=0, total=0;
        while (runner.next()) {
            if ( runner.current()->isValid() )
                valid++;
            total++;
        }
        if (total) {
            ru->setStockableArea( cHeightPixelArea * valid);
            if (valid>0 && ru->id()==-1) {
                qDebug() << "Warning: a resource unit has id=-1 but stockable area (id was set to 0)!!! ru: " << ru->boundingBox() << "with index" << ru->index();
                ru->setID(0);
                // test-code
                //GridRunner<HeightGridValue> runner(*mHeightGrid, ru->boundingBox());
                //while (runner.next()) {
                //    qDebug() << mHeightGrid->cellCenterPoint(mHeightGrid->indexOf( runner.current() )) << ": " << runner.current()->isValid();
                //}

            }
        } else
            throw IException("calculateStockableArea: resource unit without pixels!");

    }
}


/// Force the creation of stand statistics.
/// - stocked area (for resourceunit-areas)
/// - ru - statistics
void Model::createStandStatistics()
{
    calculateStockedArea();
    foreach(ResourceUnit *ru, mRU) {
        ru->addTreeAgingForAllTrees();
        ru->createStandStatistics();
    }
}


/// execute the javascript expression \p expression in the model context.
bool Model::executeJavascript(const QString expression)
{
    if (management())
        return management()->executeScript(expression).isEmpty();
    else
        return false;
}
