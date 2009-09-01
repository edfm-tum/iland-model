/** @class GlobalSettings
  This class contains various global structures/definitions. This class is a Singleton and accessed via the static instance() function.
  @par various (textual) meta data (SettingMetaData)

  @par global database connections
  There are two defined global database connections dbin() and dbout() with the names "in" and "out".
  They are setup with setupDatabaseConnection(). Currently, only SQLite DBs are supported.
  Use dbin() and dbout() to faciliate those database connections:
  @code
  ...
  QSqlQuery query(GlobalSettings::instance()->dbin());
  query.exec(...);
  ...
  @endcode

  @par Helpers with file Paths
  the actual project file is parsed for path defined in the <path> section.
  Use the path() function to expand a @p fileName to a iLand-Path. To check if a file exists, you could
  use fileExists().
  Available paths:
  - home: the project's home directory. All other directories can be defined relative to this dir.
  - lip: path for the storage of LIP (aka binary Stamp files) (default: home/lip)
  - database: base path for SQLite database files (default: home/database)
  - temp: path for storage of temporary files (default: home/temp)
  - log: storage for log-files (default: home/log)
  - exe: the path to the executable file.
  @code
  // home is "e:/iland/test", temp is "c:\temp" and log is omitted in project-file:
  QString p;
  p = Globals->path("somestuff.txt", "temp"); // > c:\temp\somestuff.txt
  p = Globals->path("e:\averyspecial\place.txt", "temp"); // -> e:\averyspecial\place.txt
                                                          //    (abs. path is not changed)
  p = Globals->path("log123.txt", "log"); // -> e:/iland/test/log/log123.txt (default for log)
  @endcode

*/
#include <QtCore>
#include <QtXml>
#include <QtSql>

#include "global.h"
#include "helper.h"
#include "xmlhelper.h"
#include "globalsettings.h"
#include "settingmetadata.h"

// debug macro helpers
void dbg_helper(const char *where, const char *what,const char* file,int line)
{
    qDebug() << "Warning in " << where << ":"<< what << ". (file: " << file << "line:" << line;
}
void dbg_helper_ext(const char *where, const char *what,const char* file,int line, const QString &s)
{
    qDebug() << "Warning in " << where << ":"<< what << ". (file: " << file << "line:" << line << "more:" << s;
}



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

        SettingMetaData *md = new SettingMetaData(SettingMetaData::typeFromName(elt.attribute("type", "invalid")), // type
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
    qDebug() << "setup database connection" << dbname << "to" << fileName;
    //db.setDatabaseName(":memory:");
    db.setDatabaseName(fileName);
    if (!db.open()) {
        Helper::msg(QString("Unable to establish a database <%2> connection to %1.\n"
                     "This software needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit.").arg(fileName, dbname));
        return false;
    }
    return true;
}


///////// Path functions

void GlobalSettings::setupDirectories(QDomElement pathNode)
{
    mFilePath.clear();
    mFilePath.insert("exe", QCoreApplication::applicationDirPath());
    XmlHelper xml(pathNode);
    QString homePath = xml.value("home", QCoreApplication::applicationDirPath());
    mFilePath.insert("home", homePath);
    // make other paths relativ to "home" if given as relative paths
    mFilePath.insert("lip", path(xml.value("lip", "lip"), "home"));
    mFilePath.insert("database", path(xml.value("database", "database"), "home"));
    mFilePath.insert("temp", path(xml.value("temp", "temp"), "home"));
    mFilePath.insert("log", path(xml.value("log", "log"), "home"));
    qDebug() << "current File Paths:" << mFilePath;
}

/** extend the file to a full absoulte path of the given type (temp, home, ...).
  If @p file is already an absolute path, nothing is done.
  */
QString GlobalSettings::path(const QString &fileName, const QString &type)
{
    QFileInfo fileinfo(fileName);
    if (fileinfo.isAbsolute())
        return fileName;

    QDir d;
    if (mFilePath.contains(type))
        d.setPath(mFilePath.value(type));
    else {
        qDebug() << "GlobalSettings::path() called with unknown type" << type;
        d = QDir::currentPath();
    }

    return d.filePath(fileName);
}

/// returns true if file @p fileName exists.
bool GlobalSettings::fileExists(const QString &fileName, const QString &type)
{
    QString name = path(fileName, type);

    if (!QFile::exists(name)) {
        qDebug() << "Path" << fileName << "(expanded to:)"<< name << "does not exist!";
        return false;
    }
    return true;
}


void GlobalSettings::loadProjectFile(const QString &fileName)
{
    qDebug() << "Loading Project file" << fileName;
    if (!QFile::exists(fileName))
        throw IException(QString("The project file %1 does not exist!").arg(fileName));
    mXml.loadFromFile(fileName);

}

