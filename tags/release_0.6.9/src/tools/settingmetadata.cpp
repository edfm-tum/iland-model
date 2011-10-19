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

