#include "bite_global.h"
#include "biteengine.h"


// BITE
#include "biteagent.h"
#include "bitescript.h"

// iLand specific
#include "globalsettings.h"
#include "helper.h"


Q_LOGGING_CATEGORY(bite, "bite")

Q_LOGGING_CATEGORY(biteSetup, "bite.setup")


namespace BITE {

BiteEngine *BiteEngine::mInstance=nullptr;


BiteEngine::BiteEngine()
{
}



BiteEngine::~BiteEngine()
{

    mInstance=nullptr;
}

void BiteEngine::setup()
{
    QLoggingCategory::setFilterRules("bite.debug=true\n" \
                                     "bite.setup.debug=true"); // enable *all*

    resetErrors();

    // setup scripting
    mScript.setup(this);


    // now load the javascript and execute
    QString file_name = GlobalSettings::instance()->path(GlobalSettings::instance()->settings().value("modules.bite.file"));

    QString code = Helper::loadTextFile(file_name);
    qCDebug(biteSetup) << "Loading script file" << file_name;
    QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate(code, file_name);
    if (result.isError()) {
        int lineno = result.property("lineNumber").toInt();
        QStringList code_lines = code.replace('\r', "").split('\n'); // remove CR, split by LF
        QString code_part;
        for (int i=std::max(0, lineno - 5); i<std::min(lineno+5, code_lines.count()); ++i)
            code_part.append(QString("%1: %2 %3\n").arg(i).arg(code_lines[i]).arg(i==lineno?"  <---- [ERROR]":""));
        qCCritical(biteSetup) << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.property("lineNumber").toInt() << ":" << result.toString() << ":\n" << code_part;
        throw IException("BITE Error in Javascript (Please check the logfile): " + result.toString());
    }




    if (mHasScriptError) {
        qCCritical(bite) << "Error in setup of BITE engine:" << mErrorStack.join("\n");
        throw IException("BITE-Error (check also the log): \n" + mErrorStack.join("\n"));
    }
}

BiteAgent *BiteEngine::agentByName(QString name)
{
    for (auto *b : mAgents)
        if (b->name() == name)
            return b;
    return nullptr;
}

QStringList BiteEngine::agentNames()
{
    QStringList names;
    for (auto *b : mAgents)
        names.push_back(b->name());
    return names;
}

QJSEngine *BiteEngine::scriptEngine()
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();

}

void BiteEngine::run()
{
    resetErrors();

    for (auto *b : mAgents) {
        try {
        b->run();
        } catch (const IException &e) {
            throw IException(QString("Error in execution of the Bite agent '%1': %2").arg(b->name()).arg(e.message()));
        }
        if (mHasScriptError) {
            qCCritical(bite) << "Error in setup of BITE engine:" << mErrorStack.join("\n");
            throw IException("BITE-Erro (check also the log): \n" + mErrorStack.join("\n"));
        }
    }
}

void BiteEngine::error(QString error_msg)
{
    mErrorStack.push_back(error_msg);
    mHasScriptError = true;
}




QJSValue BiteEngine::valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage)
{
   if (!js_value.hasOwnProperty(key)) {
       if (!errorMessage.isEmpty())
           throw IException(QString("Error: required key '%1' not found. In: %2 (JS: %3)").arg(key).arg(errorMessage).arg(BiteScript::JStoString(js_value)));
       else if (default_value.isEmpty())
           return QJSValue();
       else
           return default_value;
   }
   return js_value.property(key);
}

void BiteEngine::resetErrors()
{
    mErrorStack.clear();
    mHasScriptError = false;
}


} // end namespace
