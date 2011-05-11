#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#include <QtCore>

class Climate;
class SpeciesSet;
class CSVFile;
class GisGrid;

/** Environment specifes the geographical properties of the landscape.
    This is achieved by specifying (user defined) values (e.g. soil depth) for each resource unit.
    Resource units are specified by x/y indices.
    see http://iland.boku.ac.at/initialize+the+landscape */
class Environment
{
public:
    Environment();
    ~Environment();
    bool isSetup() const { return mInfile!=0; }
    /// switch to 'grid-mode': load the grid provided by @p grid_file_name
    bool setGridMode(const QString &grid_file_name);
    // setup
    void setDefaultValues(Climate *climate, SpeciesSet *speciesSet) {mCurrentClimate=climate; mCurrentSpeciesSet=speciesSet; }
    bool loadFromString(const QString &source);
    bool loadFromFile(const QString &fileName);
    QList<Climate*> climateList() const { return mClimate; } ///< created climates.
    QList<SpeciesSet*> speciesSetList() const { return mSpeciesSets; } ///< created species sets
    // access
    void setPosition(const QPointF position); ///< set position (metric coordinates). Subsequent calls to retriever functions are for the current location.
    Climate *climate() const { return mCurrentClimate;} ///< get climate at current pos
    SpeciesSet *speciesSet() const {return mCurrentSpeciesSet;} ///< get species set on current pos
    int currentID() const { return mCurrentID; } ///< current grid id (in grid mode, otherwise 0)

private:
    bool mGridMode;
    int mCurrentID; ///< in grid mode, current id is the (grid) ID of the resource unit
    Climate *mCurrentClimate; ///< climate at current location
    SpeciesSet *mCurrentSpeciesSet; ///< species set at current location
    QList<Climate*> mClimate; ///< created climates.
    QList<SpeciesSet*> mSpeciesSets; ///< created species sets
    QStringList mKeys;
    QHash<QString, int> mRowCoordinates;
    QHash<QString, void*> mCreatedObjects;
    CSVFile *mInfile;
    GisGrid *mGrid;

};

#endif // ENVIRONMENT_H
