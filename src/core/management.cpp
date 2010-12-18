#include "global.h"
#include "management.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "expressionwrapper.h"

#include "climateconverter.h"
#include "csvfile.h"
#include "scriptglobal.h"

#include <QtScript>
#include <QTextEdit>
QObject *Management::scriptOutput = 0;

QScriptValue script_debug(QScriptContext *ctx, QScriptEngine *eng)
{
    QString value;
    for (int i = 0; i < ctx->argumentCount(); ++i) {
        if (i > 0)
            value.append(" ");
        value.append(ctx->argument(i).toString());
    }
    if (Management::scriptOutput) {
        QTextEdit *e = qobject_cast<QTextEdit*>(Management::scriptOutput);
        if (e)
            e->append(value);
    } else {
        qDebug() << "Script:" << value;
    }
    return eng->undefinedValue();
}

QScriptValue script_include(QScriptContext *ctx, QScriptEngine *eng)
{
    QString fileName = ctx->argument(0).toString();
    QString path =GlobalSettings::instance()->path(fileName, "script") ;
    QString includeFile=Helper::loadTextFile(path);
    eng->evaluate(includeFile, fileName);
    if (eng->hasUncaughtException())
        qDebug() << "Error in include:" << eng->uncaughtException().toString();
    return QScriptValue();
}

QScriptValue script_alert(QScriptContext *ctx, QScriptEngine *eng)
{
    QString value = ctx->argument(0).toString();
    Helper::msg(value);
    return eng->undefinedValue();
}
// global output function
QString Management::executeScript(QString cmd)
{
    DebugTimer t("execute javascript");
    if (mEngine)
        mEngine->evaluate(cmd);
    if (mEngine->hasUncaughtException()) {
        //int line = mEngine->uncaughtExceptionLineNumber();
        QString msg = QString( "Script Error occured: %1\n").arg( mEngine->uncaughtException().toString());
        msg+=mEngine->uncaughtExceptionBacktrace().join("\n");
        return msg;
    } else {
        return QString();
    }
}

Management::Management()
{
    // setup the engine
    mEngine = new QScriptEngine();
    QScriptValue objectValue = mEngine->newQObject(this);
    QScriptValue dbgprint = mEngine->newFunction(script_debug);
    QScriptValue sinclude = mEngine->newFunction(script_include);
    QScriptValue alert = mEngine->newFunction(script_alert);
    mEngine->globalObject().setProperty("management", objectValue);
    mEngine->globalObject().setProperty("print",dbgprint);
    mEngine->globalObject().setProperty("include",sinclude);
    mEngine->globalObject().setProperty("alert", alert);

    // globals object: instatiate here, but ownership goes to script engine
    ScriptGlobal *global = new ScriptGlobal();
    QScriptValue glb = mEngine->newQObject(global,QScriptEngine::ScriptOwnership);
    mEngine->globalObject().setProperty("Globals", glb);
    // other object types
    ClimateConverter::addToScriptEngine(*mEngine);
    CSVFile::addToScriptEngine(*mEngine);


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
        int index = irandom(0, trees.count()-1);
        trees[index]->remove();
        trees.removeAt(index);
    }
    mRemoved += to_kill;
}


void Management::kill()
{
    for (int i=0;i<mTrees.count();i++)
        mTrees[i].first->remove();
    mTrees.clear();
}

// from the range percentile range pctfrom to pctto (each 1..100)
int Management::kill(int pctfrom, int pctto, int number)
{
    if (mTrees.isEmpty())
        return 0;
    int index_from = limit(int(pctfrom/100. * mTrees.count()), 0, mTrees.count());
    int index_to = limit(int(pctto/100. * mTrees.count()), 0, mTrees.count()-1);
    if (index_from>=index_to)
        return 0;
    qDebug() << "attempting to remove" << number << "trees between indices" << index_from << "and" << index_to;
    int i;
    int count = number;
    if (index_to-index_from <= number)  {
        // kill all
        for (i=index_from; i<index_to; i++)
            mTrees.at(i).first->remove();
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
            mTrees[rnd_index].first->remove();
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

int Management::filter(QVariantList idList)
{
    QVector<int> ids;
    foreach(const QVariant &v, idList)
        if (!v.isNull())
            ids << v.toInt();
//    QHash<int, int> ids;
//    foreach(const QVariant &v, idList)
//        ids[v.toInt()] = 1;

    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    while (tp!=mTrees.end()) {
        if (!ids.contains(tp->first->id()) )
            tp = mTrees.erase(tp);
        else
            tp++;
    }
    qDebug() << "Management::filter by id-list:" << mTrees.count();
    return mTrees.count();
}

int Management::filter(QString filter)
{
    TreeWrapper tw;
    Expression expr(filter,&tw);
    qDebug() << "filtering with" << filter;
    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    while (tp!=mTrees.end()) {
        tw.setTree(tp->first);
        if (!expr.calculate(tw))
            tp = mTrees.erase(tp);
        else
            tp++;
    }
    return mTrees.count();
}

int Management::load(int ruindex)
{
    Model *m = GlobalSettings::instance()->model();
    ResourceUnit *ru = m->ru(ruindex);
    if (!ru)
        return -1;
    mTrees.clear();
    for (int i=0;i<ru->trees().count();i++)
        mTrees.push_back(QPair<Tree*,double>(ru->tree(i), 0.));
    return mTrees.count();
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

void Management::loadFromTreeList(QList<Tree*>tree_list)
{
    mTrees.clear();
    for (int i=0;i<tree_list.count();++i)
        mTrees.append(QPair<Tree*, double>(tree_list[i], 0.));
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
