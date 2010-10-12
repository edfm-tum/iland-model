#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QtCore>
#include <QtSql>
#include <QtSql/QSqlDatabase>

#include "settingmetadata.h"
#include "xmlhelper.h"
#include "../3rdparty/MersenneTwister.h"

// use faster method to concatenate strings (see qt - documentation on QString)
#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS

typedef QList<QVariant> DebugList;

class Model;
class OutputManager;

/// General settings and globally available data
class GlobalSettings
{
public:
    // singleton-access
    static GlobalSettings *instance() { if (mInstance) return mInstance; mInstance = new GlobalSettings(); return mInstance; }
    ~GlobalSettings();
    // Access
    // model and clock
    void setModel(Model *model) {mModel = model; }
    Model *model() const { return mModel; }
    int currentYear() const { return mRunYear; }
    void setCurrentYear(const int year) { mRunYear = year; }
    // debugging fain grained debug outputs
    enum DebugOutputs { dTreeNPP=1, dTreePartition=2, dTreeGrowth=4,
                        dStandNPP=8, dWaterCycle=16, dDailyResponses=32, dEstablishment=64, dSnagDynamics=128 }; ///< defines available debug output types.
    void setDebugOutput(const int debug) { mDebugOutputs = GlobalSettings::DebugOutputs(debug); }
    void setDebugOutput(const DebugOutputs dbg, const bool enable=true); ///< enable/disable a specific output type.
    bool isDebugEnabled(const DebugOutputs dbg) {return int(dbg) & mDebugOutputs;} ///< returns true, if a specific debug outut type is enabled.
    int currentDebugOutput() const { return mDebugOutputs; }
    DebugList &debugList(const int ID, const DebugOutputs dbg); ///< returns a ref to a list ready to be filled with debug output of a type/id combination.
    const QList<DebugList> debugLists(const int ID, const DebugOutputs dbg); ///< return a list of debug outputs
    QStringList debugListCaptions(const DebugOutputs dbg); ///< returns stringlist of captions for a specific output type
    QList<QPair<QString, QVariant> > debugValues(const int ID); ///< all debug values for object with given ID
    void clearDebugLists(); ///< clear all debug data
    QStringList debugDataTable(GlobalSettings::DebugOutputs type, const QString separator); ///< output for all available items (trees, ...) in table form
    // database
    QSqlDatabase dbin() { return QSqlDatabase::database("in"); }
    QSqlDatabase dbout() { return QSqlDatabase::database("out"); }
    QSqlDatabase dbclimate() { return QSqlDatabase::database("climate"); }
    // setting-meta-data
    /// access an individual SettingMetaData named @p name.
    const SettingMetaData *settingMetaData(const QString &name);
    /// retrieve the default value of the setting @p name.
    QVariant settingDefaultValue(const QString &name);
    QList<QString> settingNames() { return mSettingMetaData.keys(); } ///< retrieve list of all names of settings.

    // path and directory
    QString path(const QString &fileName, const QString &type="home");
    bool fileExists(const QString &fileName, const QString &type="home");

    // xml project file
    const XmlHelper &settings() const { return mXml; }

    // setup and maintenance

    // xml project settings
    void loadProjectFile(const QString &fileName);

    // meta data of settings
    void loadSettingsMetaDataFromFile(const QString &fileName);
    void loadSettingsMetaDataFromXml(const QDomElement &topNode);

    // Database connections
    bool setupDatabaseConnection(const QString& dbname, const QString &fileName, bool fileMustExist);
    void clearDatabaseConnections(); ///< shutdown and clear connections
    // output manager
    OutputManager *outputManager() { return mOutputManager; }

    // path
    void setupDirectories(QDomElement pathNode, const QString &projectFilePath);

    MTRand& randomGenerator(); // get a random generator instance per thread

private:
    GlobalSettings(); // private ctor
    static GlobalSettings *mInstance;
    Model *mModel;
    OutputManager *mOutputManager;
    int mRunYear;

    // special debug outputs
    QMultiHash<int, DebugList> mDebugLists;
    int mDebugOutputs; // "bitmap" of enabled debugoutputs.

    SettingMetaDataList mSettingMetaData; ///< storage container (QHash) for settings.
    QHash<QString, QString> mFilePath; ///< storage for file paths
    QHash<QThread*, MTRand> mRandomGenerators; /// store a random generator for each thread

    XmlHelper mXml; ///< xml-based hierarchical settings
};

// constants
// We assume:
// Light-Grid: 2x2m
// Height-Grid: 10x10m
// Resource-Unit: 100x100m
const int cPxSize = 2; // size of light grid
const int cRUSize = 100; // size of resource unit
const int cPxPerHeight = 5; // 10 / 2
const int cPxPerRU = 50; // 100/2
const int cHeightPerRU = 10; // 100/10

// other constants
const double biomassCFraction = 0.5; // fraction of (dry) biomass which is carbon

/// shortcut to the GlobalSettings Singleton object.
#define Globals (GlobalSettings::instance())

#endif // GLOBALSETTINGS_H
