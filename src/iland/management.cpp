#include "global.h"
#include "management.h"
#include "helper.h"
#include <QtScript>

Management::Management()
{
    // setup the engine
    mEngine = new QScriptEngine();
    QScriptValue objectValue = mEngine->newQObject(this);
    mEngine->globalObject().setProperty("management", objectValue);
}

Management::~Management()
{
    delete mEngine;
}

void Management::loadScript(const QString &fileName)
{
    QString program = Helper::loadTextFile(fileName);
    if (program.isEmpty())
        return;

    mEngine->evaluate(program);
    qDebug() << "management script loaded";
    if (mEngine->hasUncaughtException())
        qDebug() << "Script Error occured: " << mEngine->uncaughtExceptionBacktrace();

}

void Management::remain(int number)
{
    qDebug() << "remain called (number): " << number;
}

void Management::kill(int number)
{
}

void Management::run()
{
    qDebug() << "Management::run() called";
    QScriptValue mgmt = mEngine->globalObject().property("manage");
    int year = GlobalSettings::instance()->currentYear();
    mgmt.call(QScriptValue(), QScriptValueList()<<year);
    if (mEngine->hasUncaughtException())
        qDebug() << "Script Error occured: " << mEngine->uncaughtExceptionBacktrace();

}
