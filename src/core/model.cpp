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

/** clear() frees all ressources allocated with the run of a simulation.

  */
void Model::clear()
{
    // clear ressource units
    qDeleteAll(mRU);
    mRU.clear();
    qDeleteAll(mSpeciesSets);
    mSpeciesSets.clear();
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
}
