/** @class GlobalSettings
  This class contains various global structures/definitions. This class is a Singleton and accessed via the static instance() function.
  @caption various (textual) meta data (SettingMetaData)

  @caption global database connections
  There are two defined global database connections dbin() and dbout() with the names "in" and "out".
  They are setup with setupDatabaseConnection(). Currently, only SQLite DBs are supported.
  Use dbin() and dbout() to faciliate those database connections:
  @code
  ...
  QSqlQuery query(GlobalSettings::instance()->dbin());
  query.exec(...);
  ...
  @endcode
*/
#include <QtCore>
#include <QtXml>
#include <QtSql>

#include "globalsettings.h"
#include "settingmetadata.h"
#include "helper.h"

GlobalSettings *GlobalSettings::mInstance = 0;

GlobalSettings::GlobalSettings()
{
}


GlobalSettings::~GlobalSettings()
{
    // meta data... really clear ressources...
    qDeleteAll(mSettingMetaData.values());
    mInstance = NULL;
}

/** retrieve a const pointer to a stored SettingMetaData object.
 if @p name is not found, a NULL returned.
 */
const SettingMetaData *GlobalSettings::settingMetaData(const QString &name)
{
    if (mSettingMetaData.contains(name)) {
        return mSettingMetaData[name];
    }
    return NULL;
}

QVariant GlobalSettings::settingDefaultValue(const QString &name)
{
    const SettingMetaData *smd = settingMetaData(name);
    if (smd)
        return smd->defaultValue();
    return QVariant(0);
}

void GlobalSettings::loadSettingsMetaDataFromFile(const QString &fileName)
{
    QString metadata = Helper::loadTextFile(fileName);
}

QString childText(QDomElement &elem, const QString &name, const QString &def="") {
    QDomElement e = elem.firstChildElement(name);
    if (elem.isNull())
        return def;
    else
        return e.text();
}

/** Load setting meta data from a piece of XML.
    @p topNode is a XML node, that contains the "setting" nodes as childs:
    @code
    <topnode>
    <setting>...</setting>
    <setting>...</setting>
    ...
    </topnode>
    @endcode
  */
void GlobalSettings::loadSettingsMetaDataFromXml(const QDomElement &topNode)
{
    mSettingMetaData.clear();
    if (topNode.isNull())
        WARNINGRETURN( "GlobalSettings::loadSettingsMetaDataFromXml():: no globalsettings section!");

    QString settingName;
    QDomElement elt = topNode.firstChildElement("setting");
    for (; !elt.isNull(); elt = elt.nextSiblingElement("setting")) {
        settingName = elt.attribute("name", "invalid");
        if (mSettingMetaData.contains(settingName))
            WARNINGRETURN( "GlobalSettings::loadSettingsMetaDataFromXml():: setting" << settingName << "already exists in the settings list!") ;

        SettingMetaData *md = new SettingMetaData(md->typeFromName(elt.attribute("type", "invalid")), // type
                      settingName, // name
                      childText(elt,"description"), // description
                      childText(elt, "url"), // url
                      QVariant(childText(elt,"default")));
        mSettingMetaData[settingName] = md;

        qDebug() << md->dump();
        //mSettingMetaData[settingName].dump();
    }
    qDebug() << "setup settingmetadata complete." << mSettingMetaData.count() << "items loaded.";
}

void GlobalSettings::clearDatabaseConnections()
{
    QSqlDatabase::removeDatabase("in");
    QSqlDatabase::removeDatabase("out");
}

bool GlobalSettings::setupDatabaseConnection(const QString& dbname, const QString &fileName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",dbname);
    //db.setDatabaseName(":memory:");
    db.setDatabaseName(fileName);
    if (!db.open()) {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
            qApp->tr("Unable to establish a database <%2> connection to %1.\n"
                     "This software needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit.").arg(fileName, dbname), QMessageBox::Cancel);
        return false;
    }
    return true;
}

