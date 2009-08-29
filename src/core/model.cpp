/** @class Model
  Model is the main model container.
  */

#include <QtCore>
#include <QtXml>

#include "global.h"
#include "xmlhelper.h"

#include "model.h"

#include "ressourceunit.h"
#include "speciesset.h"

Model::Model()
{
    initialize();
}

Model::~Model()
{
    clear();
}

/** Initial setup of the Model.

  */
void Model::initialize()
{

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
}


/** clear() frees all ressources allocated with the run of a simulation.

  */
void Model::clear()
{
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
    //GlobalSettings::instance()->setupDatabaseConnection("out", "");


    // (1) SpeciesSets: currently only one a global species set.
    QString speciesTableName = xml.value("species.source", "species");
    SpeciesSet *speciesSet = new SpeciesSet();
    mSpeciesSets.push_back(speciesSet);

    speciesSet->setup();

    // (2) RessourceUnits (including everything else).
    RessourceUnit *ru = new RessourceUnit();
    ru->setSpeciesSet(speciesSet);
    mRU.push_back(ru);

    setupSpace();
}
