/** @class SettingsMetaData
 This is some help text for the SettingMetaData class.
  This is some help text for the SettingMetaData class.
   This is some help text for the SettingMetaData class.

*/

#include <QtCore>
#include "SettingMetaData.h"

const QStringList SettingMetaData::mTypeNames = QStringList() << "invalid" <<  "species" << "model";

SettingMetaData::Type SettingMetaData::settingFromName(const QString &settingName)
{
    Type retType = (Type) mTypeNames.indexOf(settingName);
    if ( int(retType)<0)
        retType = SettingInvalid;
    return retType;
}

const QString SettingMetaData::settingName(const Type type)
{
    return mTypeNames.value(int(type),mTypeNames[0]);
}

SettingMetaData::SettingMetaData()
{
    mType = SettingInvalid;
}

SettingMetaData::SettingMetaData(const Type type, const QString &name, const QString &description, const QString &url, const QVariant defaultValue)
{
    mType = type;
    mName = name;
    mDescription = description;
    mUrl = url;
    mDefautValue = defaultValue;
}
