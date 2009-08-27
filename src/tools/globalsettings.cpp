#include <QtCore>
#include <QtXml>

#include "globalsettings.h"
#include "settingmetadata.h"
#include "helper.h"

GlobalSettings *GlobalSettings::mInstance = 0;

GlobalSettings::GlobalSettings()
{
}

/** retrieve a const reference to a stored SettingMetaData object.
 if @p name is not found, a reference to a data object with type Invalid is returned.
 */
const SettingMetaData &GlobalSettings::settingMetaData(const QString &name)
{

    if (mSettingMetaData.contains(name)) {
        return mSettingMetaData[name];
    }
    return mSettingMetaData["invalid"];
}

QVariant GlobalSettings::settingDefaultValue(const QString &name)
{
    return settingMetaData(name).defaultValue();
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


        SettingMetaData &md = mSettingMetaData[settingName]; // creates a default constructed entry and returns ref to it
        md.setValues( md.typeFromName(elt.attribute("type", "invalid")), // type
                      settingName, // name
                      childText(elt,"description"), // description
                      childText(elt, "url"), // url
                      QVariant(childText(elt,"default"))
                      );
        qDebug() << md.dump();
        //mSettingMetaData[settingName].dump();
    }
    qDebug() << "setup settingmetadata complete." << mSettingMetaData.count() << "items loaded.";
}
