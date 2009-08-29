#ifndef SPECIESSET_H
#define SPECIESSET_H
#include <QtSql>

#include "stampcontainer.h"

class Species;
class SpeciesSet
{
public:
    SpeciesSet();
    ~SpeciesSet();
    // access
    Species *species(const QString &speciesId) { return mSpecies.value(speciesId); }
    // maintenance
    void clear();
    QVariant var(const QString& varName);
    int loadFromDatabase(const QString &tableName);

private:

    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
    StampContainer mReaderStamp;
};

#endif // SPECIESSET_H
