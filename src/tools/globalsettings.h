#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H
#include <QtSql>
#include <QtXml>

#include "settingmetadata.h"
#include "xmlhelper.h"
#include "exception.h"

/// General settings and globally available data
class GlobalSettings
{
public:
    static GlobalSettings *instance() { if (mInstance) return mInstance; mInstance = new GlobalSettings(); return mInstance; }
    ~GlobalSettings();
    // Access
    // database
    QSqlDatabase dbin() { return QSqlDatabase::database("in"); }
    QSqlDatabase dbout() { return QSqlDatabase::database("out"); }
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
    const XmlHelper &settings() { return mXml; }

    // setup and maintenance

    // xml project settings
    void loadProjectFile(const QString &fileName);

    // meta data of settings
    void loadSettingsMetaDataFromFile(const QString &fileName);
    void loadSettingsMetaDataFromXml(const QDomElement &topNode);

    // Database connections
    bool setupDatabaseConnection(const QString& dbname, const QString &fileName);
    void clearDatabaseConnections(); ///< shutdown and clear connections

    // path
    void setupDirectories(QDomElement pathNode);

private:
    GlobalSettings(); // private ctor
    static GlobalSettings *mInstance;
    SettingMetaDataList mSettingMetaData; /// storage container (QHash) for settings.
    QHash<QString, QString> mFilePath;
    XmlHelper mXml;
};

/// shortcut to the GlobalSettings Singleton object.
#define Globals (GlobalSettings::instance())

#endif // GLOBALSETTINGS_H
