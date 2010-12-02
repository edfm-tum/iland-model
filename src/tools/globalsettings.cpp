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
#include <algorithm>
#include "global.h"
#include "helper.h"
#include "xmlhelper.h"
#include "stdint.h"

#include "settingmetadata.h"

#include "outputmanager.h"
#include "../3rdparty/MersenneTwister.h"
MTRand _mtrand;
MTRand &mtRand()
{
    return _mtrand;
}

// debug macro helpers
void dbg_helper(const char *where, const char *what,const char* file,int line)
{
    qDebug() << "Warning in " << where << ":"<< what << ". (file: " << file << "line:" << line;
}
void dbg_helper_ext(const char *where, const char *what,const char* file,int line, const QString &s)
{
    qDebug() << "Warning in " << where << ":"<< what << ". (file: " << file << "line:" << line << "more:" << s;
}

int _loglevel=0;
 // true, if detailed debug information is logged
bool logLevelDebug()
{
    return _loglevel<1;
}

// true, if only important aggreate info is logged
bool logLevelInfo()
{
    return _loglevel<2;
}

// true if only severe warnings/errors are logged.
bool logLevelWarning()
{
    return _loglevel<3;
}
void setLogLevel(int loglevel)
{
    _loglevel=loglevel;
    switch (loglevel) {
    case 0: qDebug() << "Loglevel set to Debug."; break;
    case 1: qDebug() << "Loglevel set to Info."; break;
    case 2: qDebug() << "Loglevel set to Warning."; break;
    case 3: qDebug() << "Loglevel set to Error/Quiet."; break;
    default: qDebug() << "invalid log level" << loglevel; break;
    }
}

GlobalSettings *GlobalSettings::mInstance = 0;

GlobalSettings::GlobalSettings()
{
    mDebugOutputs = 0;
    mModel = 0;
    // create output manager
    mOutputManager = new OutputManager();
}


GlobalSettings::~GlobalSettings()
{
    // meta data... really clear ressources...
    qDeleteAll(mSettingMetaData.values());
    mInstance = NULL;
    delete mOutputManager;
    // clear all databases
    clearDatabaseConnections();
}

// debugging
void GlobalSettings::setDebugOutput(const GlobalSettings::DebugOutputs dbg, const bool enable)
{
    if (enable)
        mDebugOutputs |= int(dbg);
    else
        mDebugOutputs &= int(dbg) ^ 0xffffffff;
}


void GlobalSettings::clearDebugLists()
{
    mDebugLists.clear();
}

QMutex debugListMutex;
DebugList &GlobalSettings::debugList(const int ID, const DebugOutputs dbg)
{
    QMutexLocker m(&debugListMutex); // serialize creation of debug outputs
    DebugList dbglist;
    dbglist << ID << dbg << currentYear();
    QMultiHash<int, DebugList>::iterator newitem = mDebugLists.insert(ID, dbglist);
    return *newitem;
}
bool debuglist_sorter (const DebugList &i,const DebugList &j)
{
    return (i[0].toInt() < j[0].toInt());
}
const QList<DebugList> GlobalSettings::debugLists(const int ID, const DebugOutputs dbg)
{
    QList<DebugList> result_list;
    if (ID==-1) {
        foreach(DebugList list, mDebugLists)
            if (list.count()>2)  // contains data
                if (int(dbg)==-1 || (list[1]).toInt() & int(dbg) ) // type fits or is -1 for all
                    result_list << list;
    } else {
        // search a specific id
        QMultiHash<int, DebugList>::iterator res = mDebugLists.find(ID);
        while (res != mDebugLists.end() && res.key() == ID)  {
            DebugList &list = res.value();
            if (list.count()>2)  // contains data
                if (int(dbg)==-1 || (list[1]).toInt() & int(dbg) ) // type fits or is -1 for all
                    result_list << list;
            ++res;
        }
    }
    // sort result list
    std::sort(result_list.begin(), result_list.end(), debuglist_sorter);
    return result_list;
}

QStringList GlobalSettings::debugListCaptions(const DebugOutputs dbg)
{
    QStringList treeCaps = QStringList() << "Id" << "Species" << "Dbh" << "Height" << "x" << "y" << "ru_index" << "LRI"
                           << "mWoody" << "mRoot" << "mFoliage" << "LA";
    if ( int(dbg)==0)
        return treeCaps;
    switch(dbg) {
    case dTreeNPP: return QStringList() << "id" << "type" << "year" << treeCaps
                << "LRI_modRU" <<"lightResponse" << "effective_area" << "raw_gpp" << "gpp" << "npp" << "aging";

    case dTreeGrowth: return QStringList() << "id" << "type" << "year" <<  treeCaps
                << "netNPPStem" << "massStemOld" << "hd_growth" << "factor_diameter" << "delta_d_estimate" << "d_increment";

    case dTreePartition: return QStringList() << "id" << "type" << "year" << treeCaps
                << "npp_kg" << "apct_foliage" << "apct_wood" << "apct_root"
                << "delta_foliage" << "delta_woody" << "delta_root" << "mNPPReserve" << "netStemInc" << "stress_index";

    case dStandNPP: return QStringList() << "id" << "type" << "year" << "standnpp" << "hach" << "hech";

    case dWaterCycle: return QStringList() << "id" << "type" << "year" << "date" << "ruindex" << "temp" << "vpd" << "prec" << "rad" << "combined_response"
                << "after_intercept" << "after_snow" << "et_canopy" << "evapo_intercepted"
                << "content" << "psi_kpa" << "excess_mm" << "snow_height";

    case dDailyResponses: return QStringList() << "id" << "type" << "year" << "species" << "date" << "RU_index"
                << "waterResponse" << "tempResponse" << "VpdResponse" << "Radiation of day" << "util.Radiation";

    case dEstablishment: return QStringList() << "id" << "type" << "year" << "species" << "RU_index"
                 << "avgProbDensity" << "TACAminTemp" << "TACAchill" << "TACAfrostFree" << "TACAgdd" << "TACAFrostAfterBud" << "TACAAbioticEnv"
                 << "fEnvYr" << "avgLIFValue" <<"N_Established" << "Living_cohorts" << "averageHeight" << "averageAge" << "avgDeltaHPot" << "avgDeltaHRealized"
                 << "Added" << "Died" << "Recruited" << "refRatio";
    case dSnagDynamics: return QStringList() << "id" << "type" << "year" << "RU_index"
                << "SnagState_c" << "TotalC_in" << "TotalC_toAtm" << "SWDtoDWD_c" << "SWDtoDWD_n" << "toLabile_c" << "toLabile_n" << "toRefr_c" << "toRefr_n" << "SWDtoSoil_c" << "SWDtoSoil_n"
                << "swd1_c" << "swd1_n" << "swd1_count" << "swd1_tsd" << "toSwd1_c" << "toSwd1_n" // pool of small dbhs
                << "swd2_c" << "swd2_n" << "swd2_count" << "swd2_tsd" << "toSwd2_c" << "toSwd2_n" // standing woody debris medium dbhs
                << "swd3_c" << "swd3_n" << "swd3_count" << "swd3_tsd" << "toSwd3_c" << "toSwd3_n"// large trees
                << "branch1_c" << "branch1_n" << "branch2_c" << "branch2_n" << "branch3_c" << "branch3_n" << "branch4_c" << "branch4_n" << "branch5_c" << "branch5_n";
    }
    return QStringList() << "invalid debug output!";
}

QStringList GlobalSettings::debugDataTable(GlobalSettings::DebugOutputs type, const QString separator)
{

    GlobalSettings *g = GlobalSettings::instance();
    QList<DebugList> ddl = g->debugLists(-1, type); // get all debug data

    QStringList result;
    result << g->debugListCaptions(type).join(separator);
    foreach (const DebugList &l, ddl) {
        QString line;
        int c=0;
        foreach(const QVariant &value, l) {
            if (c++)
                line+=separator;
            line += value.toString();
        }
        result << line;
    }
    return result;
}

QList<QPair<QString, QVariant> > GlobalSettings::debugValues(const int ID)
{

    QList<QPair<QString, QVariant> > result;
    QMultiHash<int, DebugList>::iterator res = mDebugLists.find(ID);
    while (res != mDebugLists.end() && res.key() == ID)  {
        DebugList &list = res.value();
        if (list.count()>2) { // contains data
           QStringList cap = debugListCaptions( DebugOutputs(list[1].toInt()) );
           result.append(QPair<QString, QVariant>("Debug data", "Debug data") );
           int first_index = 3;
           if (list[3]=="Id")  // skip default data fields (not needed for drill down)
               first_index=14;
           for (int i=first_index;i<list.count();++i)
               result.append(QPair<QString, QVariant>(cap[i], list[i]));
        }
        ++res;
    }
    return result;
}

/** retrieve a const pointer to a stored SettingMetaData object.
 if @p name is not found, NULL is returned.
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
    QSqlDatabase::removeDatabase("climate");
}

bool GlobalSettings::setupDatabaseConnection(const QString& dbname, const QString &fileName, bool fileMustExist)
{

    QSqlDatabase::database(dbname).close(); // close database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",dbname); // addDatabase replaces a connection with the same name
    qDebug() << "setup database connection" << dbname << "to" << fileName;
    //db.setDatabaseName(":memory:");
    if (fileMustExist)
        if (!QFile::exists(fileName))
            throw IException("Error setting up database connection: file " + fileName + " does not exist!");
    db.setDatabaseName(fileName);
    if (!db.open()) {
        throw IException(QString("Error in setting up the database connection <%2> connection to file %1.\n").arg(fileName, dbname));
    }
    if (dbname == "out") {
        // some special commands (pragmas: see also: http://www.sqlite.org/pragma.html)
        db.exec("pragma temp_store(2)"); // temp storage in memory
        db.exec("pragma synchronous(1)"); // medium synchronization between memory and disk (faster than "full", more than "none")
    }
    return true;
}


///////// Path functions

void GlobalSettings::setupDirectories(QDomElement pathNode, const QString &projectFilePath)
{
    mFilePath.clear();
    mFilePath.insert("exe", QCoreApplication::applicationDirPath());
    XmlHelper xml(pathNode);
    QString homePath = xml.value("home", projectFilePath);
    mFilePath.insert("home", homePath);
    // make other paths relativ to "home" if given as relative paths
    mFilePath.insert("lip", path(xml.value("lip", "lip"), "home"));
    mFilePath.insert("database", path(xml.value("database", "database"), "home"));
    mFilePath.insert("temp", path(xml.value("temp", "temp"), "home"));
    mFilePath.insert("log", path(xml.value("log", "log"), "home"));
    mFilePath.insert("script", path(xml.value("script", "script"), "home"));
    mFilePath.insert("init", path(xml.value("init", "init"), "home"));
    mFilePath.insert("output", path(xml.value("output", "output"), "home"));
    qDebug() << "current File Paths:" << mFilePath;
}

/** extend the file to a full absoulte path of the given type (temp, home, ...).
  If @p file is already an absolute path, nothing is done. @sa setupDirectories().
  */
QString GlobalSettings::path(const QString &fileName, const QString &type)
{
    if (!fileName.isEmpty()) {
        QFileInfo fileinfo(fileName);
        if (fileinfo.isAbsolute())
            return QDir::cleanPath(fileName);
    }

    QDir d;
    if (mFilePath.contains(type))
        d.setPath(mFilePath.value(type));
    else {
        qDebug() << "GlobalSettings::path() called with unknown type" << type;
        d = QDir::currentPath();
    }

    return QDir::cleanPath(d.filePath(fileName)); // let QDir build the correct path
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
    setupDirectories(mXml.node("system.path"),QFileInfo(fileName).path());

}

MTRand& GlobalSettings::randomGenerator()
{
    QThread *this_thread = QThread::currentThread();
    if (mRandomGenerators.contains(this_thread))
        return mRandomGenerators[this_thread];
    qDebug() << "add random generator for " << this_thread;
    MTRand &rand=mRandomGenerators[this_thread]; // this inserts a "empty" instance
    uint32_t s = irandom(0,100000000);
    rand.seed(s); // to initialize, draw a random number from the "default" source -->
    return rand;
}
