/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

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
