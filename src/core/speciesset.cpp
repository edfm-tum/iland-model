#include <QtCore>
#include <QtSql>
#include "global.h"
#include "speciesset.h"
#include "species.h"

SpeciesSet::SpeciesSet()
{
    mSetupQuery = 0;
}

SpeciesSet::~SpeciesSet()
{
   clear();
}

void SpeciesSet::clear()
{
    qDeleteAll(mSpecies.values());
    mSpecies.clear();
}


/** loads active species from a database table and creates/setups the species.
    The function uses the global database-connection.
  */
int SpeciesSet::loadFromDatabase(const QString &tableName)
{
    QSqlQuery query(GlobalSettings::instance()->dbin());
    mSetupQuery = &query;
    QString sql = QString("select * from %1").arg(tableName);
    query.exec(sql);
    clear();
    qDebug() << "attempting to load a species set from" << tableName;
    while (query.next()) {
        if (var("active").toInt()==0)
            continue;

        Species *s = new Species(this); // create
        // call setup routine (which call SpeciesSet::var() to retrieve values
        s->setup();

        mSpecies.insert(s->id(), s); // store
    } // while query.next()
    qDebug() << "loaded" << mSpecies.count() << "active species:";
    qDebug() << mSpecies.keys();

    mSetupQuery = 0;
    return mSpecies.count();

}
/** retrieves variables from the datasource available during the setup of species.
  */
QVariant SpeciesSet::var(const QString& varName)
{
    Q_ASSERT(mSetupQuery!=0);

    int idx = mSetupQuery->record().indexOf(varName);
    if (idx>=0)
        return mSetupQuery->value(idx);
    // lookup in defaults
    qDebug() << "variable" << varName << "not found - using default.";
    return GlobalSettings::instance()->settingDefaultValue(varName);
}
