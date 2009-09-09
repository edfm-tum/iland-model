/** @class Model
  Model is the main model container.
  */
#include "global.h"
#include "model.h"

#include "xmlhelper.h"
#include "helper.h"
#include "ressourceunit.h"
#include "speciesset.h"
#include "standloader.h"
#include "tree.h"

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
   GlobalSettings::instance()->setRunYear(0);
    //
}

void Model::setupSpace()
{
    const XmlHelper &xml = GlobalSettings::instance()->settings();
    double cellSize = xml.value("world.cellSize", "2").toDouble();
    double width = xml.value("world.width", "100").toDouble();
    double height = xml.value("world.height", "100").toDouble();
    double buffer = xml.value("world.buffer", "50").toDouble();
    qDebug() << QString("setup of the world: %1x%2m with cell-size=%3m and %4m buffer").arg(width).arg(height).arg(cellSize).arg(buffer);


    QRectF total_grid(QPointF(-buffer, -buffer), QPointF(width+buffer, height+buffer));
    qDebug() << "setup grid rectangle:" << total_grid;

    mGrid = new FloatGrid(total_grid, cellSize);
    mGridList.push_back(mGrid);
    mGrid->initialize(1.f); // set to unity...
    mHeightGrid = new FloatGrid(total_grid, cellSize*5);
    mGridList.push_back(mHeightGrid); // could be solved better...
    mHeightGrid->initialize(0.f); // zero grid

    // simple case: create ressource units in a regular grid.
    mRUmap.clear();
    if (xml.hasNode("world.ressourceUnitsAsGrid")) {
        mRUmap.setup(QRectF(0., 0., width, height),100.);
        RessourceUnit **p=mRUmap.begin();
        RessourceUnit *new_ru;
        mRU.first()->setBoundingBox(QRectF(0., 0., 100., 100.)); // the first
        *p = mRU.first(); // store first RU in grid.
        SpeciesSet *species_set = (*p)->speciesSet(); // copy the species sets
        p++; // no need to create the first...
        int ru_index = 1;
        for (; p!=mRUmap.end(); ++p) {
            QRectF r = mRUmap.cellRect(mRUmap.indexOf(p));
            new_ru = new RessourceUnit(ru_index++);
            new_ru->setSpeciesSet(species_set);
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
        qDebug() << "created a grid of RessourceUnits: count=" << mRU.count();
        // setup the helper that does the multithreading
        threadRunner.setup(mRU);
        threadRunner.setMultithreading(xml.value("system.multithreading", "false") == "true");
        threadRunner.print();

    }
    mSetup = true;
}


/** clear() frees all ressources allocated with the run of a simulation.

  */
void Model::clear()
{
    mSetup = false;
    qDebug() << "Model clear: attempting to clear" << mRU.count() << "RU, " << mSpeciesSets.count() << "SpeciesSets, " << mGridList.count() << "Grids.";
    // clear ressource units
    qDeleteAll(mRU); // delete ressource units (and trees)
    mRU.clear();

    qDeleteAll(mSpeciesSets); // delete species sets
    mSpeciesSets.clear();

    qDeleteAll(mGridList); // delete all grids
    mGridList.clear();

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
    // fix file path
    g->setupDirectories(xml.node("path"));
    g->clearDatabaseConnections();
    // database connections: reset
    GlobalSettings::instance()->clearDatabaseConnections();
    // input and output connection
    QString dbPath = g->path( xml.value("database.in"), "database");
    g->fileExists(dbPath);
    GlobalSettings::instance()->setupDatabaseConnection("in", dbPath);


    // (1) SpeciesSets: currently only one a global species set.
    QString speciesTableName = xml.value("species.source", "species");
    SpeciesSet *speciesSet = new SpeciesSet();
    mSpeciesSets.push_back(speciesSet);

    speciesSet->setup();

    // (2) RessourceUnits (including everything else).
    RessourceUnit *ru = new RessourceUnit(0);
    ru->setSpeciesSet(speciesSet);
    mRU.push_back(ru);

    setupSpace();
}


RessourceUnit *Model::ru(QPointF &coord)
{
    if (!mRUmap.isEmpty() && mRUmap.coordValid(coord))
        return mRUmap.valueAt(coord);
    return ru(); // default RU if only one created
}

void Model::beforeRun()
{
    // initialize stands
    DebugTimer loadtrees("load trees");
    StandLoader loader(this);
    loader.processInit();
    Tree::lafactor = GlobalSettings::instance()->settings().paramValue("lafactor",1.);

    Tree::setGrid(mGrid, mHeightGrid);
    applyPattern();
    readPattern();

}

void Model::runYear()
{
    // process a cycle
    Tree::setGrid(mGrid, mHeightGrid);
    applyPattern();
    readPattern();

    grow();

    //test();

    GlobalSettings::instance()->setRunYear(GlobalSettings::instance()->runYear()+1);
}

void Model::afterStop()
{
    // do some cleanup
}

RessourceUnit* nc_applyPattern(RessourceUnit *unit)
{
    unit->newYear(); // reset state of some variables

    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator tend = unit->trees().end();

    // height dominance grid
    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).heightGrid(); // just do it ;)
    }

    // light concurrence influence
    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).applyStamp(); // just do it ;)
    }
    return unit;
}

RessourceUnit *nc_readPattern(RessourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();

    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).readStampMul(); // multipliactive approach
    }
    return unit;
}

RessourceUnit *nc_grow(RessourceUnit *unit)
{
    QVector<Tree>::iterator tit;
    QVector<Tree>::iterator  tend = unit->trees().end();

    for (tit=unit->trees().begin(); tit!=tend; ++tit) {
        (*tit).grow(); //
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
    mGrid->initialize(1.);
    mHeightGrid->initialize(0.);

    threadRunner.run(nc_applyPattern);
}

void Model::readPattern()
{
    DebugTimer t("readPattern()");
    threadRunner.run(nc_readPattern);
}

void Model::grow()
{
    { DebugTimer t("growRU()");
        foreach(RessourceUnit *ru, mRU) {
            ru->production();
        }
    }

    DebugTimer t("grow()");
    threadRunner.run(nc_grow);
}
