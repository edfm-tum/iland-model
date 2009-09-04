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

  @par Fine-Grained debugging outputs
  The enumeration DebugOutputs defines a list of realms (uses binary notation: 1,2,4,8,...!).
  Use setDebugOutput() to enable/disable such an output. Use isDebugEnabled() to test inside the
  code if the generation of debug output for a specific type is enabled. Internally, this is a single
  bitwise operation which is very fast.
  Call debugLists() to retrieve a list of lists of data that fit specific criteria.
  @code
    // use something like that somewhere in a tree-growth-related routine:
    DBGMODE(
       if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreeGrowth) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreeGrowth); // get a ref to the list
            out << hd_growth << factor_diameter << delta_d_estimate << d_increment;   // fill with data
       }
    ); // only in debugmode
  @endcode

*/
#include "globalsettings.h"
#include <QtCore>
#include <QtXml>
#include <QtSql>

#include "global.h"
#include "helper.h"
#include "xmlhelper.h"

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
    mDebugOutputs = 0;
}


GlobalSettings::~GlobalSettings()
{
    // meta data... really clear ressources...
    qDeleteAll(mSettingMetaData.values());
    mInstance = NULL;
}

// debugging
void GlobalSettings::setDebugOutput(const GlobalSettings::DebugOutputs dbg, const bool enable)
{
    if (enable)
        mDebugOutputs |= int(dbg);
    else
        mDebugOutputs &= int(dbg) ^ 0xffffffff;
}

QString GlobalSettings::makeDebugKey(const int id, const int type)
{
    return QString("%1 %2").arg(id).arg(type);
}

DebugList &GlobalSettings::debugList(const int ID, const DebugOutputs dbg)
{
    const QString &key = makeDebugKey(ID, int(dbg));
    if (mDebugLists.contains(key)) {
        DebugList &dbglist = mDebugLists[key];
        return dbglist; // rely here on Qt's implicit sharing!
    }
    DebugList &dbglist = mDebugLists[key];
    dbglist << ID << dbg; // the first two elements should always be the ID and the type.
    return dbglist;
}

const QList<DebugList> GlobalSettings::debugLists(const int ID, const DebugOutputs dbg)
{
    QList<DebugList> result_list;
    foreach(DebugList list, mDebugLists)
        if (list.count()>2)  // contains data
            if (ID==-1 ||  list[0]==ID) // id fits or is -1 for all
                if (int(dbg)==-1 || (list[1]).toInt() & int(dbg) ) // type fits or is -1 for all
                    result_list << list;

    return result_list;
}

QStringList GlobalSettings::debugListCaptions(const DebugOutputs dbg)
{
    switch(dbg) {
        case dTreeGrowth: return QStringList() << "id" << "type" << "growth" << "hach" << "hech";
        case dTreePartition: return QStringList() << "id" << "type" << "partition" << "hach" << "hech";
            case dStandNPP: return QStringList() << "id" << "type" << "standnpp" << "hach" << "hech";
    }
    return QStringList() << "invalid debug output!";
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

