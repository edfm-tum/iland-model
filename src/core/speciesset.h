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
    const StampContainer &readerStamps() { return mReaderStamp; }
    // maintenance
    void clear();
    QVariant var(const QString& varName);
    int setup();

private:

    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
    StampContainer mReaderStamp;
};

#endif // SPECIESSET_H
