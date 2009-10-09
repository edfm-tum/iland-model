#include "global.h"
#include "management.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "expressionwrapper.h"

#include "climateconverter.h"

#include <QtScript>
#include <QTextEdit>
QObject *Management::scriptOutput = 0;

QScriptValue script_debug(QScriptContext *ctx, QScriptEngine *eng)
 {
     QString value = ctx->argument(0).toString();
     if (Management::scriptOutput) {
         QTextEdit *e = qobject_cast<QTextEdit*>(Management::scriptOutput);
         if (e)
             e->append(value);
     } else {
         qDebug() << "Script:" << value;
     }
     return QScriptValue();
     //return ctx->thisObject().property(name);
 }

// global output function
QString Management::executeScript(QString cmd)
{
    DebugTimer t("execute javascript");
    if (mEngine)
        mEngine->evaluate(cmd);
    if (mEngine->hasUncaughtException())
        return QString( "Script Error occured: %1").arg( mEngine->uncaughtException().toString());
    else
        return QString();
}

Management::Management()
{
    // setup the engine
    mEngine = new QScriptEngine();
    QScriptValue objectValue = mEngine->newQObject(this);
    QScriptValue dbgprint = mEngine->newFunction(script_debug);
    mEngine->globalObject().setProperty("management", objectValue);
    mEngine->globalObject().setProperty("print",dbgprint);
    // other object types
    ClimateConverter::addToScriptEngine(*mEngine);

}

Management::~Management()
{
    delete mEngine;
}

void Management::loadScript(const QString &fileName)
{
    mScriptFile = fileName;
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
        int index = irandom(0, trees.count());
        trees[index]->die();
        trees.removeAt(index);
    }
    mRemoved += to_kill;
}


// from the range percentile range pctfrom to pctto (each 1..100)
int Management::kill(int pctfrom, int pctto, int number)
{
    if (mTrees.isEmpty())
        return 0;
    int index_from = limit(int(pctfrom/100. * mTrees.count()), 0, mTrees.count());
    int index_to = limit(int(pctto/100. * mTrees.count()), 0, mTrees.count());
    if (index_from>=index_to)
        return 0;
    qDebug() << "attempting to remove" << number << "trees between indices" << index_from << "and" << index_to;
    int i;
    int count = number;
    if (index_to-index_from <= number)  {
        // kill all
        for (i=index_from; i<index_to; i++)
            mTrees.at(i).first->die();
        count = index_to - index_from;
    } else {
        // kill randomly the provided number
        int cancel = 1000;
        while(number>=0) {
            int rnd_index = irandom(index_from, index_to);
            if (mTrees[rnd_index].first->isDead()) {
                if (--cancel<0) {
                    qDebug() << "Management::kill: canceling search." << number << "trees left.";
                    count-=number; // not all trees were killed
                    break;
                }
                continue;
            }
            cancel = 1000;
            number--;
            mTrees[rnd_index].first->die();
        }
    }
    qDebug() << count << "removed.";
    return count; // killed
}

void Management::run()
{
    mTrees.clear();
    mRemoved=0;
    qDebug() << "Management::run() called";
    QScriptValue mgmt = mEngine->globalObject().property("manage");
    int year = GlobalSettings::instance()->currentYear();
    mgmt.call(QScriptValue(), QScriptValueList()<<year);
    if (mEngine->hasUncaughtException())
        qDebug() << "Script Error occured: " << mEngine->uncaughtExceptionBacktrace();

    if (mRemoved>0) {
        foreach(ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
           ru->cleanTreeList();
   }
}


int Management::load(QString filter)
{
    TreeWrapper tw;
    Model *m = GlobalSettings::instance()->model();
    mTrees.clear();
    AllTreeIterator at(m);
    if (filter.isEmpty()) {
        while (Tree *t=at.next())
            if (!t->isDead())
                mTrees.push_back(QPair<Tree*, double>(t, 0.));
    } else {
        Expression expr(filter,&tw);
        qDebug() << "filtering with" << filter;
        while (Tree *t=at.next()) {
            tw.setTree(t);
            if (!t->isDead() && expr.execute())
                mTrees.push_back(QPair<Tree*, double>(t, 0.));
        }
    }
    return mTrees.count();
}

bool treePairValue(const QPair<Tree*, double> &p1, const QPair<Tree*, double> &p2)
{
    return p1.second < p2.second;
}

void Management::sort(QString statement)
{
    TreeWrapper tw;
    Expression sorter(statement, &tw);
    // fill the "value" part of the tree storage with a value for each tree
    for (int i=0;i<mTrees.count(); ++i) {
        tw.setTree(mTrees.at(i).first);
        mTrees[i].second = sorter.execute();
   }
   // now sort the list....
   qSort(mTrees.begin(), mTrees.end(), treePairValue);
}

double Management::percentile(int pct)
{
    if (mTrees.count()==0)
        return -1.;
    int idx = int( (pct/100.) * mTrees.count());
    if (idx>=0 && idx<mTrees.count())
        return mTrees.at(idx).second;
    else
        return -1;
}
