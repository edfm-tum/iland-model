#ifndef MODEL_H
#define MODEL_H
#include <QtCore>
#include <QtXml>

#include "global.h"
#include "globalsettings.h"
#include "grid.h"
#include "helper.h"
#include "xmlhelper.h"
#include "speciesset.h"
#include "ressourceunit.h"


class Model
{
public:
    Model();
    ~Model();
    // access
    RessourceUnit *ru() { return mRU.front(); }

    // setup/maintenance
    void clear(); ///< free ressources
    void loadProject(); ///< setup and load a project

private:
    void initialize(); ///< basic startup without creating a simulation
    void setupSpace(); ///< setup the "world"(spatial grids, ...), create ressource units

    /// container holding all ressource units
    QList<RessourceUnit*> mRU;
    /// container holding all species sets
    QList<SpeciesSet*> mSpeciesSets;
    // global grids...
    FloatGrid *mGrid;
    FloatGrid *mHeightGrid;
    QList<FloatGrid*> mGridList;
};

#endif // MODEL_H
