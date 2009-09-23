#include "global.h"
#include "management.h"
#include "helper.h"
#include "model.h"
#include "ressourceunit.h"
#include "tree.h"


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
    Model *m = GlobalSettings::instance()->model();
    AllTreeIterator at(m);
    QList<Tree*> trees;
    while (Tree *t=at.next())
        trees.push_back(t);
    int to_kill = trees.count() - number;
    qDebug() << trees.count() << " standing, targetsize" << number << ", hence " << to_kill << "trees to remove";
    for (int i=0;i<to_kill;i++) {
        int index = random(0, trees.count());
        trees[index]->die();
        trees.removeAt(index);
    }
    mRemoved += to_kill;
}

void Management::kill(int number)
{
}

void Management::run()
{
    mRemoved=0;
    qDebug() << "Management::run() called";
    QScriptValue mgmt = mEngine->globalObject().property("manage");
    int year = GlobalSettings::instance()->currentYear();
    mgmt.call(QScriptValue(), QScriptValueList()<<year);
    if (mEngine->hasUncaughtException())
        qDebug() << "Script Error occured: " << mEngine->uncaughtExceptionBacktrace();

    if (mRemoved>0) {
        foreach(RessourceUnit *ru, GlobalSettings::instance()->model()->ruList())
           ru->cleanTreeList();
   }
}
