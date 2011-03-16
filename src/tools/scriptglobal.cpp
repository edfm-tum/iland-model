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

MapGridWrapper::MapGridWrapper(QObject *parent)
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

void MapGridWrapper::saveAsImage(QString file)
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
        Helper::saveToTextFile(file_name, result);
        qDebug() << "saved grid to " << file_name;
        return true;
    }
    qDebug() << "could not save gridToFile because" << grid_type << "is not a valid grid.";
    return false;

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

