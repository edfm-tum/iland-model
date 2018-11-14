#include "bite_global.h"
#include "biteengine.h"


// BITE
#include "biteagent.h"
#include "bitescript.h"

// iLand specific
#include "globalsettings.h"
#include "helper.h"
#include "modelcontroller.h"
#include "debugtimer.h"

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
    mRunning = true;

    QString code = Helper::loadTextFile(file_name);
    qCDebug(biteSetup) << "Loading script file" << file_name;
    QJSValue result = GlobalSettings::instance()->scriptEngine()->evaluate(code, file_name);
    mRunning = false;
    if (result.isError()) {
        int lineno = result.property("lineNumber").toInt();
        QStringList code_lines = code.replace('\r', "").split('\n'); // remove CR, split by LF
        QString code_part;
        for (int i=std::max(0, lineno - 5); i<std::min(lineno+5, code_lines.count()); ++i)
            code_part.append(QString("%1: %2 %3\n").arg(i).arg(code_lines[i]).arg(i==lineno?"  <---- [ERROR]":""));
        qCCritical(biteSetup).noquote() << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.property("lineNumber").toInt() << ":" << result.toString() << ":\n" << code_part;
        throw IException("BITE Error in Javascript (Please check the logfile): " + result.toString());
    }




    if (mHasScriptError) {
        qCCritical(bite) << "Error in setup of BITE engine:" << mErrorStack.join("\n");
        throw IException("BITE-Error (check also the log): \n" + mErrorStack.join("\n"));
    }
}

void BiteEngine::addAgent(BiteAgent *new_agent)
{
    BiteAgent *a = agentByName(new_agent->name());
    if (a) {
        qCInfo(bite) << "adding an agent with a name already in use. Deleting the *old* agent.";
        mAgents.removeOne(a);
        // remove agent from UI
        GlobalSettings::instance()->controller()->removePaintLayers(a);
        delete a;
    }
    mAgents.push_back(new_agent);
    // add agent to UI
    QStringList varlist = new_agent->wrapper()->getVariablesList();
    for (int i=0;i<varlist.size();++i)
        varlist[i] = QString("Bite:%1 - %2").arg(new_agent->name()).arg(varlist[i]);
    GlobalSettings::instance()->controller()->addPaintLayers(new_agent, varlist);

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

Grid<double> *BiteEngine::preparePaintGrid(QObject *handler, QString name)
{
    // check if handler is a valid agent
    BiteAgent *ba = qobject_cast<BiteAgent*>(handler);
    if (!ba)
        return nullptr;
    // name: is still Bite::<agentname> - <varname>
    QStringList l = name.split(" - ");
    if (l.size() != 2)
        return nullptr;
    ba->updateDrawGrid(l[1]);
    return ba->baseDrawGrid();
}



void BiteEngine::run()
{
    DebugTimer t("Bite:run");
    resetErrors();

    for (auto *b : mAgents) {
        try {
            mRunning = true;
        b->run();
        } catch (const IException &e) {
            mRunning = false;
            throw IException(QString("Error in execution of the Bite agent '%1': %2").arg(b->name()).arg(e.message()));
        }
        mRunning = false;

        if (mHasScriptError) {
            qCCritical(bite) << "Error in setup of BITE engine:" << mErrorStack.join("\n");
            throw IException("BITE-Error (check also the log): \n" + mErrorStack.join("\n"));
        }
    }
}

void BiteEngine::error(QString error_msg)
{
    mErrorStack.push_back(error_msg);
    mHasScriptError = true;
    if (!mRunning)
        throw IException("Bite Error: " + error_msg);
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
