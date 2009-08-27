#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include "settingmetadata.h"

/// General settings and globally available data
class GlobalSettings
{
public:
    static GlobalSettings *instance() { if (mInstance) return mInstance; mInstance = new GlobalSettings(); return mInstance; }
    ~GlobalSettings();
    /// meta data of settings
    void loadSettingsMetaDataFromFile(const QString &fileName);
    void loadSettingsMetaDataFromXml(const QDomElement &topNode);

    const SettingMetaData *settingMetaData(const QString &name);
    QVariant settingDefaultValue(const QString &name);
    QList<QString> settingNames() { return mSettingMetaData.keys(); }

private:
    GlobalSettings(); // private ctor
    static GlobalSettings *mInstance;
    SettingMetaDataList mSettingMetaData; /// storage container (QHash) for settings.
};

/// shortcut to the GlobalSettings Singleton object.
#define Globals (GlobalSettings::instance())

#endif // GLOBALSETTINGS_H
