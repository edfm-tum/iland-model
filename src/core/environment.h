#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#include <QtCore>

class Climate;
class SpeciesSet;
class CSVFile;


class Environment
{
public:
    Environment();
    ~Environment();
    // setup
    bool loadFromString(const QString &source);
    bool loadFromFile(const QString &fileName);
    // access
    void setPosition(const QPointF position); ///< set position (metric coordinates). Subsequent calls to retriever functions are for the current location.
    Climate *climate(); ///< get climate at current pos
    SpeciesSet *speciesSet(); ///< get species set on current pos

private:
    void createModelElements();
    QList<Climate*> mClimate; ///< created climates.
    QList<SpeciesSet*> mSpeciesSets; ///< created species sets
    QStringList mKeys;
    QHash<QString, int> mRowCoordinates;
    CSVFile *mInfile;

};

#endif // ENVIRONMENT_H
