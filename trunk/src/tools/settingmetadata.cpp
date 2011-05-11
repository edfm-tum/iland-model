/** @class SettingMetaData
 This is some help text for the SettingMetaData class.
*/

#include <QtCore>
#include "settingmetadata.h"

const QStringList SettingMetaData::mTypeNames = QStringList() << "invalid" <<  "species" << "model";

SettingMetaData::Type SettingMetaData::typeFromName(const QString &settingName)
{
    Type retType = (Type) mTypeNames.indexOf(settingName);
    if ( int(retType)<0)
        retType = SettingInvalid;
    return retType;
}

const QString SettingMetaData::typeName(const Type type) const
{
    return mTypeNames.value(int(type),mTypeNames[0]);
}

SettingMetaData::SettingMetaData()
{
    mType = SettingInvalid;
}

SettingMetaData::SettingMetaData(const Type type, const QString &name, const QString &description, const QString &url, const QVariant defaultValue)
{
    setValues(type, name, description, url, defaultValue);
}

void SettingMetaData::setValues(const Type type, const QString &name, const QString &description, const QString &url, const QVariant defaultValue)
{
    mType = type;
    mName = name;
    mDescription = description;
    mUrl = url;
    mDefaultValue = defaultValue;
}

QString SettingMetaData::dump() const
{
    QString res = QString("Name: %1\nType: %2 *** Default Value: %5 *** Url: %3 *** Memory address: %6 \nDescription: %4").
                  arg(mName,
                      typeName(mType),
                      mUrl,
                      mDescription).
                  arg(mDefaultValue.toString()).
                  arg(int(this), 0, 16);
    return res;
}

