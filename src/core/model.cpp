/** @class Model
  Main object of the iLand model composited of various sub models / sub components.
  */
#include "global.h"
#include "model.h"

#include "xmlhelper.h"
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
  @endcode
  */
Tree *AllTreeIterator::next()
{

    if (!mTreeEnd) {
        // initialize to first ressource unit
        mRUIterator = mModel->ruList().constBegin();
        if (mRUIterator == mModel->ruList().constEnd()) return NULL;
        mTreeEnd = &((*mRUIterator)->trees().back()) + 1; // let end point to "1 after end" (STL-style)
        mCurrent = &((*mRUIterator)->trees().front());
    }
    if (mCurrent==mTreeEnd) {
        mRUIterator++; // switch to next RU
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
    //
}

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
    mHeightGrid->wipe();
    Tree::setGrid(mGrid, mHeightGrid);

    // simple case: create ressource units in a regular grid.
    mRUmap.clear();
    if (xml.valueBool("resourceUnitsAsGrid")) {
        mRUmap.setup(QRectF(0., 0., width, height),100.);
        ResourceUnit **p=mRUmap.begin();
        ResourceUnit *new_ru;
        mRU.first()->setBoundingBox(QRectF(0., 0., 100., 100.)); // the first
        *p = mRU.first(); // store first RU in grid.
        SpeciesSet *species_set = (*p)->speciesSet(); // copy the species sets
        Climate *clim = (*p)->climate(); // copy the climate
        p++; // no need to create the first...
        int ru_index = 1;
        for (; p!=mRUmap.end(); ++p) {
            QRectF r = mRUmap.cellRect(mRUmap.indexOf(p));
            new_ru = new ResourceUnit(ru_index++);
            new_ru->setSpeciesSet(species_set);
            new_ru->setClimate(clim);
            new_ru->setup();
            mRU.append(new_ru); // store in list
            new_ru->setBoundingBox(r);
        }
        ru_index=0;
        // now store the pointers in the grid.
        // Important: This has to be done after the mRU-QList is complete - otherwise pointers would
        // point to invalid memory when QList's memory is reorganized (expanding)
        for (p=mRUmap.begin();p!=mRUmap.end(); ++p) {
            *p = mRU.value(ru_index++);
        }
        qDebug() << "created a grid of ResourceUnits: count=" << mRU.count();
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

    mGrid = 0;
    mHeightGrid = 0;
    mManagement = 0;

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
    // input and output connection
    QString dbPath = g->path( xml.value("system.database.in"), "database");
    GlobalSettings::instance()->setupDatabaseConnection("in", dbPath, true);
    dbPath = g->path( xml.value("system.database.out"), "database");
    GlobalSettings::instance()->setupDatabaseConnection("out", dbPath, false);
    dbPath = g->path( xml.value("system.database.climate"), "database");
    GlobalSettings::instance()->setupDatabaseConnection("climate", dbPath, true);

    mSettings.loadModelSettings();
    mSettings.print();

    // (1) SpeciesSets: currently only one a global species set.
    QString speciesTableName = xml.value("model.species.source", "species");
    SpeciesSet *speciesSet = new SpeciesSet();
    mSpeciesSets.push_back(speciesSet);

    speciesSet->setup();

    // (2) ResourceUnits (including everything else).
    ResourceUnit *ru = new ResourceUnit(0);
    ru->setSpeciesSet(speciesSet);

    // Climate...
    Climate *c = new Climate();
    mClimates.push_back(c);
    ru->setClimate(c);

    ru->setup();

    mRU.push_back(ru);
    setupSpace();

    // (3) additional issues
    // (3.1) management
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
    return ru(); // default RU if only one created
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
        c->setup();

    }

    { DebugTimer loadinit("load standstatistics");
    Tree::setGrid(mGrid, mHeightGrid);
    applyPattern();
    readPattern();

    createStandStatistics();
    }
}

void Model::runYear()
{
    DebugTimer t("Model::runYear()");
    foreach(Climate *c, mClimates)
        c->nextYear();

    // process a cycle of individual growth
    Tree::setGrid(mGrid, mHeightGrid);
    applyPattern();
    readPattern();
    grow();

    // management
    if (mManagement)
        mManagement->run();

    OutputManager *om = GlobalSettings::instance()->outputManager();
    om->execute("tree");
    om->execute("stand");
    om->execute("production_month");
    om->execute("dynamicstand");

    GlobalSettings::instance()->setCurrentYear(GlobalSettings::instance()->currentYear()+1);
}

void Model::afterStop()
{
    // do some cleanup
}

ResourceUnit* nc_applyPattern(ResourceUnit *unit)
{
    unit->newYear(); // reset state of some variables

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
    unit->yearEnd();
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
                ru->countStockedPixel( (*i).count>0 );
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
