#ifndef SPECIESSET_H
#define SPECIESSET_H
#include <QtSql>

class Species;
class SpeciesSet
{
public:
    SpeciesSet();
    ~SpeciesSet();
    // access
    inline const Species *species(const QString &speciesId);
    // maintenance
    void clear();
    QVariant var(const QString& varName);
    int loadFromDatabase(const QString &tableName);

private:

    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
};

#endif // SPECIESSET_H
