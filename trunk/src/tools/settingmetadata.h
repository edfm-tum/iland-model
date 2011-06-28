#ifndef SettingMetaData_H
#define SettingMetaData_H

/** This helper class holds meta data (description, Urls, etc) about model settings.
   Various types of settings (species, model, ...) are stored together.
*/
class SettingMetaData
{
public:
    /// type of the setting
    enum Type { SettingInvalid, SpeciesSetting, ModelSetting };
    SettingMetaData();
    SettingMetaData(const Type type, const QString &name, const QString &description, const QString &url, const QVariant defaultValue);
    void setValues(const Type type, const QString &name, const QString &description, const QString &url, const QVariant defaultValue);
    /// converts a string to a setting Type
    static SettingMetaData::Type typeFromName(const QString &settingName);
    /// convert a Type to a string.
    const QString typeName(const Type type) const;
    // getters
    QVariant defaultValue() const { return mDefaultValue; }
    const QString &url() const { return mUrl; }
    const QString &name() const { return mName; }
    const QString &description() const { return mDescription; }
    /// dump content of meta data to a String
    QString dump() const;
private:
    //SettingMetaData(const SettingMetaData &other); // private copoy ctor
    static const QStringList mTypeNames;
    Type mType;
    QString mName;
    QString mDescription;
    QString mUrl;
    QVariant mDefaultValue;
};

typedef  QHash<QString, SettingMetaData*> SettingMetaDataList;

#endif // SettingMetaData_H