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
}

/** sets up the simulation space.
*/
void Model::setupSpace()
{
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.world"));
    double cellSize = xml.value("cellSize", "2").toDouble();
    double width = xml.value("width", "100").toDouble();
    double height = xml.value("height", "100").toDouble();
    double buffer = xml.value("buffer", "50").toDouble();
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


    // simple case: create ressource units in a regular grid.
    mRUmap.clear();
    if (xml.valueBool("resourceUnitsAsGrid")) {
        mRUmap.setup(QRectF(0., 0., width, height),100.); // Grid, that holds positions of resource units
        ResourceUnit **p=mRUmap.begin(); // ptr to ptr!
        ResourceUnit *new_ru; // ptr to ptr!
        int ru_index = 0;
        for (p=mRUmap.begin(); p!=mRUmap.end(); ++p) {
            QRectF r = mRUmap.cellRect(mRUmap.indexOf(p));
            mEnvironment->setPosition( r.center() ); // if environment is 'disabled' default values from the project file are used.
            new_ru = new ResourceUnit(ru_index++); // create resource unit
            new_ru->setClimate( mEnvironment->climate() );
            new_ru->setSpeciesSet( mEnvironment->speciesSet() );
            new_ru->setup();
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
        // setup of the project area mask
        if (xml.valueBool("areaMask.enabled", false) && xml.hasNode("areaMask.imageFile")) {
            // to be extended!!! e.g. to load ESRI-style text files....
            // setup a grid with the same size as the height grid...
            FloatGrid tempgrid((int)mHeightGrid->cellsize(), mHeightGrid->sizeX(), mHeightGrid->sizeY());
            QString fileName = GlobalSettings::instance()->path(xml.value("areaMask.imageFile"));
            loadGridFromImage(fileName, tempgrid); // fetch from image
            for (int i=0;i<tempgrid.count(); i++)
                mHeightGrid->valueAtIndex(i).setValid( tempgrid.valueAtIndex(i)>0.99 );
            qDebug() << "loaded project area mask from" << fileName;
        }

        // setup the helper that does the multithreading
        threadRunner.setup(mRU);
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

    mGrid = 0;
    mHeightGrid = 0;
    mManagement = 0;
    mEnvironment = 0;
    mTimeEvents = 0;

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

    // load environment (multiple climates, speciesSets, ...
    if (mEnvironment)
        delete mEnvironment;
    mEnvironment = new Environment();

    if (xml.valueBool("model.world.environmentEnabled", false)) {
        QString envFile = xml.value("model.world.environmentFile");
        if (!mEnvironment->loadFromFile(envFile))
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
    if (xml.valueBool("model.world.timeEventsEnabled", false)) {
        mTimeEvents = new TimeEvents();
        mTimeEvents->loadFromFile(xml.value("model.world.timeEventsFile")) ;
    }

    setupSpace();
    if (mRU.isEmpty())
        throw IException("Setup of Model: no resource units present!");

    // (3) additional issues

    // (3.2) management
    QString mgmtFile = xml.value("model.management.file");
    if (!mgmtFile.isEmpty() && xml.valueBool("model.management.enabled")) {
        mManagement = new Management();
        QString path = GlobalSettings::instance()->path(mgmtFile, "script");
        mManagement->loadScript(path);
        qDebug() << "setup management using script" << path;
    }
}


ResourceUnit *Model::ru(QPointF &coord)
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

void Model::beforeRun()
{
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

    createStandStatistics();
    }
    // setup output database
    initOutputDatabase();

    // outputs to create with inital state (without any growth) are called here:
    GlobalSettings::instance()->outputManager()->execute("stand"); // year=0
    GlobalSettings::instance()->outputManager()->execute("tree"); // year=0

    GlobalSettings::instance()->setCurrentYear(1); // set to first year
}

/** Main model runner.
  The sequence of actions is as follows:
  (1) Load the climate of the new year
  (2) Reset statistics for resource unit as well as for dead/managed trees
  (3) Invoke Management. Also the dedicated spot for the actions of other disturbances.
  (4) *after* that, calculate Light patterns
  (5) 3PG on stand level, tree growth. Clear stand-statistcs before they are filled by single-tree-growth. calculate water cycle (with LAIs before management)
  (6) calculate statistics for the year
  (7) write database outputs
  */
void Model::runYear()
{
    DebugTimer t("Model::runYear()");
    // execute scheduled events for the current year
    if (mTimeEvents)
        mTimeEvents->run();

    // load the next year of the climate database
    foreach(Climate *c, mClimates)
        c->nextYear();

    // reset statistics
    foreach(ResourceUnit *ru, mRU)
        ru->newYear();

    // management
    if (mManagement)
        mManagement->run();

    // process a cycle of individual growth
    applyPattern(); // create Light Influence Patterns
    readPattern(); // readout light state of individual trees
    grow(); // let the trees grow (growth on stand-level, tree-level, mortality)

    // calculate statistics
    foreach(ResourceUnit *ru, mRU)
        ru->yearEnd();

    // create outputs
    OutputManager *om = GlobalSettings::instance()->outputManager();
    om->execute("tree"); // single tree output
    om->execute("stand"); //resource unit level x species
    om->execute("production_month"); // 3pg responses growth per species x RU x month
    om->execute("dynamicstand"); // output with user-defined columns (based on species x RU)
    om->execute("standdead"); // resource unit level x species
    om->execute("management"); // resource unit level x species

    GlobalSettings::instance()->setCurrentYear(GlobalSettings::instance()->currentYear()+1);
}



void Model::afterStop()
{
    // do some cleanup
}

ResourceUnit* nc_applyPattern(ResourceUnit *unit)
{

    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator tend = unit->trees().end();


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
}

ResourceUnit *nc_readPattern(ResourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();

    if (!GlobalSettings::instance()->settings().paramValueBool("torus")) {
        for (tit=unit->trees().begin(); tit!=tend; ++tit)
            (*tit).readLIF(); // multipliactive approach
    } else {
        for (tit=unit->trees().begin(); tit!=tend; ++tit)
            (*tit).readLIF_torus(); // do it the wraparound way
    }
    return unit;
}

ResourceUnit *nc_grow(ResourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();
    // calculate light responses
    // responses are based on *modified* values for LightResourceIndex
    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).calcLightResponse();
    }

    unit->calculateInterceptedArea();

    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).grow(); // actual growth of individual trees
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
    mHeightGrid->wipe();

    threadRunner.run(nc_applyPattern);
}

void Model::readPattern()
{
    DebugTimer t("readPattern()");
    threadRunner.run(nc_readPattern);
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
    {
        if (!settings().growthEnabled)
            return;
        DebugTimer t("growRU()");
        calculateStockedArea();

        foreach(ResourceUnit *ru, mRU) {
            ru->production();
        }
    }

    DebugTimer t("grow()");
    threadRunner.run(nc_grow); // actual growth of individual trees

    foreach(ResourceUnit *ru, mRU) {
       ru->cleanTreeList();
       //qDebug() << (b-n) << "trees died (of" << b << ").";
   }
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

/// Force the creation of stand statistics.
/// - stocked area (for resourceunit-areas)
/// - ru - statistics
void Model::createStandStatistics()
{
    calculateStockedArea();
    foreach(ResourceUnit *ru, mRU)
        ru->createStandStatistics();
}
