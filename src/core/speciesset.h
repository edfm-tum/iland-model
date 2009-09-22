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
    QList<Species*> activeSpecies() { return mActiveSpecies; }
    Species *species(const QString &speciesId) { return mSpecies.value(speciesId); }
    const Species *species(const int &index); ///< get by arbirtray index (slower than using string-id!)
    const StampContainer &readerStamps() { return mReaderStamp; }
    // maintenance
    void clear();
    QVariant var(const QString& varName);
    int setup();
    int count() const { return mSpecies.count(); }
private:
    QList<Species*> mActiveSpecies;
    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
    StampContainer mReaderStamp;
};

#endif // SPECIESSET_H
