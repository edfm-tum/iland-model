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
// for redirecting the script output
#include <QTextEdit>

#include "global.h"
#include "scriptglobal.h"
#include "model.h"
#include "globalsettings.h"
#include "helper.h"
#include "standloader.h"
#include "mapgrid.h"
#include "outputmanager.h"
#include "modelcontroller.h"
#include "grid.h"
#include "snapshot.h"

// for accessing script publishing functions
#include "climateconverter.h"
#include "csvfile.h"
#include "spatialanalysis.h"
class ResourceUnit;

/** @class ScriptGlobal
  @ingroup scripts
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

QObject *ScriptGlobal::scriptOutput = 0;

Q_SCRIPT_DECLARE_QMETAOBJECT(ScriptGlobal, QObject*)
void ScriptGlobal::addToScriptEngine(QScriptEngine &engine)
{
    // about this kind of scripting magic see: http://qt.nokia.com/developer/faqs/faq.2007-06-25.9557303148
    QScriptValue my_class = engine.scriptValueFromQMetaObject<ScriptGlobal>();
    // the script name for the object is "ClimateConverter".
    engine.globalObject().setProperty("Globals", my_class);
}

ScriptGlobal::ScriptGlobal(QObject *)
{
    mModel = GlobalSettings::instance()->model();
    // current directory
    if (mModel)
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
    return loader.loadDistributionList(content, ru, 0, "called_from_script");
}

int ScriptGlobal::addTreesOnMap(const int standID, QString content)
{
    StandLoader loader(mModel);
    return loader.loadDistributionList(content, NULL, standID, "called_from_script");
}

/*
********** MapGrid wrapper
*/

Q_SCRIPT_DECLARE_QMETAOBJECT(MapGridWrapper, QObject*)

void MapGridWrapper::addToScriptEngine(QScriptEngine &engine)
{
    // about this kind of scripting magic see: http://qt.nokia.com/developer/faqs/faq.2007-06-25.9557303148
    QScriptValue cc_class = engine.scriptValueFromQMetaObject<MapGridWrapper>();
    // the script name for the object is "Map".
    engine.globalObject().setProperty("Map", cc_class);
}

MapGridWrapper::MapGridWrapper(QObject *)
{
    mMap = const_cast<MapGrid*>(GlobalSettings::instance()->model()->standGrid());
    mCreated = false;
}

MapGridWrapper::~MapGridWrapper()
{
    if (mCreated)
        delete mMap;
}


void MapGridWrapper::load(QString file_name)
{
    if (mCreated)
        delete mMap;
    mMap = new MapGrid(file_name);
    mCreated = true;
}

bool MapGridWrapper::isValid() const
{
    return mMap->isValid();
}

void MapGridWrapper::saveAsImage(QString)
{
    qDebug() << "not implemented";
}

void MapGridWrapper::paint(double min_value, double max_value)
{
    //gridToImage(mMap->grid(), false, min_value, max_value).save(file_name);
    if (mMap) {
        if (GlobalSettings::instance()->controller())
            GlobalSettings::instance()->controller()->paintMap(mMap, min_value, max_value);

    }

}

QString MapGridWrapper::name() const
{
    if (mMap)
        return mMap->name();
    else
        return "invalid";
}
double MapGridWrapper::area(int id) {
    if (mMap && mMap->isValid())
        return mMap->area(id);
    else
        return -1;
}

bool ScriptGlobal::startOutput(QString table_name)
{
    if (table_name == "debug_dynamic") {
        GlobalSettings::instance()->controller()->setDynamicOutputEnabled(true);
        qDebug() << "started dynamic debug output";
        return true;
    }
    if (table_name.startsWith("debug_")) {
        GlobalSettings::DebugOutputs dbg = GlobalSettings::instance()->debugOutputId(table_name.mid(6));
        if (dbg==0)
            qDebug() << "cannot start debug output" << table_name << "because this is not a valid name.";
        GlobalSettings::instance()->setDebugOutput(dbg, true);
        return true;
    }
    OutputManager *om = GlobalSettings::instance()->outputManager();
    if (!om) return false;
    Output *out = om->find(table_name);
    if (!out) {
        QString err=QString("Output '%1' is not a valid output.").arg(table_name);
        if (context())
           context()->throwError(err);
        return false;
    }
    out->setEnabled(true);
    qDebug() << "started output" << table_name;
    return true;
}

bool ScriptGlobal::stopOutput(QString table_name)
{
    if (table_name == "debug_dynamic") {
        GlobalSettings::instance()->controller()->setDynamicOutputEnabled(false);
        qDebug() << "stopped dynamic debug output.";
        return true;
    }
    if (table_name.startsWith("debug_")) {
        GlobalSettings::DebugOutputs dbg = GlobalSettings::instance()->debugOutputId(table_name.mid(6));
        if (dbg==0)
            qDebug() << "cannot stop debug output" << table_name << "because this is not a valid name.";
        GlobalSettings::instance()->setDebugOutput(dbg, false);
        return true;
    }
    OutputManager *om = GlobalSettings::instance()->outputManager();
    if (!om) return false;
    Output *out = om->find(table_name);
    if (!out) {
        QString err=QString("Output '%1' is not a valid output.").arg(table_name);
        if (context())
           context()->throwError(err);
        return false;
    }
    out->setEnabled(false);
    qDebug() << "stopped output" << table_name;
    return true;
}

bool ScriptGlobal::screenshot(QString file_name)
{
    if (GlobalSettings::instance()->controller())
        GlobalSettings::instance()->controller()->saveScreenshot(file_name);
    return true;
}

void ScriptGlobal::repaint()
{
    if (GlobalSettings::instance()->controller())
        GlobalSettings::instance()->controller()->repaint();
}

void ScriptGlobal::setViewport(double x, double y, double scale_px_per_m)
{
    if (GlobalSettings::instance()->controller())
        GlobalSettings::instance()->controller()->setViewport(QPointF(x,y), scale_px_per_m);
}

// helper function...
QString heightGrid_height(const HeightGridValue &hgv) {
    return QString::number(hgv.height);
}

/// write grid to a file...
bool ScriptGlobal::gridToFile(QString grid_type, QString file_name)
{
    if (!GlobalSettings::instance()->model())
        return false;
    QString result;
    if (grid_type == "height")
        result = gridToESRIRaster(*GlobalSettings::instance()->model()->heightGrid(), *heightGrid_height);
    if (grid_type == "lif")
        result = gridToESRIRaster(*GlobalSettings::instance()->model()->grid());

    if (!result.isEmpty()) {
        file_name = GlobalSettings::instance()->path(file_name);
        Helper::saveToTextFile(file_name, result);
        qDebug() << "saved grid to " << file_name;
        return true;
    }
    qDebug() << "could not save gridToFile because" << grid_type << "is not a valid grid.";
    return false;

}

void ScriptGlobal::wait(int milliseconds)
{
    // http://stackoverflow.com/questions/1950160/what-can-i-use-to-replace-sleep-and-usleep-in-my-qt-app
    QMutex dummy;
    dummy.lock();
    QWaitCondition waitCondition;
    waitCondition.wait(&dummy, milliseconds);
}

int ScriptGlobal::addSaplingsOnMap(const MapGridWrapper *map, const int mapID, QString species, int px_per_hectare, double height)
{
    QString csv_file = QString("species;count;height\n%1;%2;%3").arg(species).arg(px_per_hectare).arg(height);
    StandLoader loader(mModel);
    try {
        loader.setMap(map->map());
        return loader.loadSaplings(csv_file, mapID, "called from script");
    } catch (const IException &e) {
        if (context())
           context()->throwError(e.message());
    }
    return 0;
}

/// saves a snapshot of the current model state (trees, soil, etc.)
/// to a dedicated SQLite database.
bool ScriptGlobal::saveModelSnapshot(QString file_name)
{
    try {
        Snapshot shot;
        QString output_db = GlobalSettings::instance()->path(file_name);
        return shot.createSnapshot(output_db);
    } catch (const IException &e) {
        if (context())
           context()->throwError(e.message());
    }
    return false;
}

/// loads a snapshot of the current model state (trees, soil, etc.)
/// from a dedicated SQLite database.
bool ScriptGlobal::loadModelSnapshot(QString file_name)
{
    try {
        Snapshot shot;
        QString input_db = GlobalSettings::instance()->path(file_name);
        return shot.loadSnapshot(input_db);
    } catch (const IException &e) {
        if (context())
           context()->throwError(e.message());
    }
    return false;
}

void ScriptGlobal::loadScript(const QString &fileName)
{
    QScriptEngine *engine = GlobalSettings::instance()->scriptEngine();

    QString program = Helper::loadTextFile(fileName);
    if (program.isEmpty())
        return;

    engine->evaluate(program);
    qDebug() << "javascript file loaded" << fileName;
    if (engine->hasUncaughtException())
        qDebug() << "Script Error occured: " << engine->uncaughtException().toString() << "\n" << engine->uncaughtExceptionBacktrace();

}

QString ScriptGlobal::executeScript(QString cmd)
{
    DebugTimer t("execute javascript");
    QScriptEngine *engine = GlobalSettings::instance()->scriptEngine();
    if (engine)
        engine->evaluate(cmd);
    if (engine->hasUncaughtException()) {
        //int line = mEngine->uncaughtExceptionLineNumber();
        QString msg = QString( "Script Error occured: %1\n").arg( engine->uncaughtException().toString());
        msg+=engine->uncaughtExceptionBacktrace().join("\n");
        return msg;
    } else {
        return QString();
    }
}

QScriptValue script_include(QScriptContext *ctx, QScriptEngine *eng)
{

    QString fileName = ctx->argument(0).toString();
    QString path =GlobalSettings::instance()->path(fileName, "script") ;
    QString includeFile=Helper::loadTextFile(path);

    ctx->setActivationObject(ctx->parentContext()->activationObject());
    ctx->setThisObject(ctx->parentContext()->thisObject());

    QScriptValue ret = eng->evaluate(includeFile, fileName);
    if (eng->hasUncaughtException())
        qDebug() << "Error in include:" << eng->uncaughtException().toString();
    return ret;
}

QScriptValue script_alert(QScriptContext *ctx, QScriptEngine *eng)
{
    QString value = ctx->argument(0).toString();
    Helper::msg(value);
    return eng->undefinedValue();
}

QScriptValue script_debug(QScriptContext *ctx, QScriptEngine *eng)
{
    QString value;
    for (int i = 0; i < ctx->argumentCount(); ++i) {
        if (i > 0)
            value.append(" ");
        value.append(ctx->argument(i).toString());
    }
    if (ScriptGlobal::scriptOutput) {
        QTextEdit *e = qobject_cast<QTextEdit*>(ScriptGlobal::scriptOutput);
        if (e)
            e->append(value);
    } else {
        qDebug() << "Script:" << value;
    }
    return eng->undefinedValue();
}

void ScriptGlobal::setupGlobalScripting()
{
    QScriptEngine *engine = GlobalSettings::instance()->scriptEngine();
    QScriptValue dbgprint = engine->newFunction(script_debug);
    QScriptValue sinclude = engine->newFunction(script_include);
    QScriptValue alert = engine->newFunction(script_alert);
    engine->globalObject().setProperty("print",dbgprint);
    engine->globalObject().setProperty("include",sinclude);
    engine->globalObject().setProperty("alert", alert);

    // other object types
    ClimateConverter::addToScriptEngine(*engine);
    CSVFile::addToScriptEngine(*engine);
    MapGridWrapper::addToScriptEngine(*engine);
    SpatialAnalysis::addToScriptEngine();

}

