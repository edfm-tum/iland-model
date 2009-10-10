#include "global.h"
#include "scriptglobal.h"
#include "model.h"
#include "globalsettings.h"
#include "helper.h"

/** @class ScriptGlobal
   This is a global interface providing useful functionality for javascripts.
  Within javascript-code an instance of this class can be accessed as "Globals" in the global scope
 (no instantiation necessary).
 @paragraph General properties

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

int ScriptGlobal::year() const
{
    return GlobalSettings::instance()->currentYear();
}

// wrapped helper functions
QString ScriptGlobal::loadTextFile(QString fileName)
{
    return Helper::loadTextFile(fileName);
}
void ScriptGlobal::saveTextFile(QString fileName, QString content)
{
    Helper::saveToTextFile(fileName, content);
}
bool ScriptGlobal::fileExists(QString fileName)
{
   return QFile::exists(fileName);
}
