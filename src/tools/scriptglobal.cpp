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

#include "global.h"
#include "scriptglobal.h"
#include "model.h"
#include "globalsettings.h"
#include "helper.h"
#include "standloader.h"

class ResourceUnit;

/** @class ScriptGlobal
   This is a global interface providing useful functionality for javascripts.
  Within javascript-code an instance of this class can be accessed as "Globals" in the global scope
 (no instantiation necessary).*/

/** \page globals Globals documentation
  Here are objects visible in the global space of javascript.
  \section sec An example section
  This page contains the subsections \ref subsection1 and \ref subsection2.
  For more info see page \ref page2.
  \subsection subsection1 The first subsection
  Text.
  \subsection subsection2 The second subsection
 - year integer. Current simulation year
 - currentDir current working directory. default value is the "script" directory defined in the project file.
  More text.
*/


Q_SCRIPT_DECLARE_QMETAOBJECT(ScriptGlobal, QObject*)
void ScriptGlobal::addToScriptEngine(QScriptEngine &engine)
{
    // about this kind of scripting magic see: http://qt.nokia.com/developer/faqs/faq.2007-06-25.9557303148
    QScriptValue my_class = engine.scriptValueFromQMetaObject<ScriptGlobal>();
    // the script name for the object is "ClimateConverter".
    engine.globalObject().setProperty("Globals", my_class);
}

ScriptGlobal::ScriptGlobal(QObject *parent)
{
    mModel = GlobalSettings::instance()->model();
    // current directory
    mCurrentDir = GlobalSettings::instance()->path(QString(), "script") + QDir::separator();
}

QVariant ScriptGlobal::setting(QString key)
{
    const XmlHelper &xml = GlobalSettings::instance()->settings();
    if (!xml.hasNode(key)) {
        qDebug() << "scriptglobal: setting key " << key << "not valid.";
        return QVariant(); // undefined???
    }
    return QVariant(xml.value(key));
}
void ScriptGlobal::set(QString key, QString value)
{
    XmlHelper &xml = const_cast<XmlHelper&>(GlobalSettings::instance()->settings());
    if (!xml.hasNode(key)) {
        qDebug() << "scriptglobal: setting key " << key << "not valid.";
        return;
    }
    xml.setNodeValue(key, value);
}

QString ScriptGlobal::defaultDirectory(QString dir)
{
    QString result = GlobalSettings::instance()->path(QString(), dir) + QDir::separator();
    return result;
}

int ScriptGlobal::year() const
{
    return GlobalSettings::instance()->currentYear();
}
int ScriptGlobal::resourceUnitCount() const
{
    Q_ASSERT(mModel!=0);
    return mModel->ruList().count();
}
// wrapped helper functions
QString ScriptGlobal::loadTextFile(QString fileName)
{
    return Helper::loadTextFile(GlobalSettings::instance()->path(fileName));
}
void ScriptGlobal::saveTextFile(QString fileName, QString content)
{
    Helper::saveToTextFile(fileName, content);
}
bool ScriptGlobal::fileExists(QString fileName)
{
   return QFile::exists(fileName);
}

/// add trees on given resource unit
/// @param content init file in a string (containing headers)
/// @return number of trees added
int ScriptGlobal::addSingleTrees(const int resourceIndex, QString content)
{
    StandLoader loader(mModel);
    ResourceUnit *ru = mModel->ru(resourceIndex);
    if (!ru)
        throw IException(QString("addSingleTrees: invalid resource unit (index: %1").arg(resourceIndex));
    int cnt = loader.loadSingleTreeList(content, ru, "called_from_script");
    qDebug() << "script: addSingleTrees:" << cnt <<"trees loaded.";
    return cnt;
}

int ScriptGlobal::addTrees(const int resourceIndex, QString content)
{
    StandLoader loader(mModel);
    ResourceUnit *ru = mModel->ru(resourceIndex);
    if (!ru)
        throw IException(QString("addTrees: invalid resource unit (index: %1").arg(resourceIndex));
    return loader.loadDistributionList(content, ru, "called_from_script");
}
