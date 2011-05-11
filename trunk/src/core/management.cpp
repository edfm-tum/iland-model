#include "global.h"
#include "management.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "expressionwrapper.h"
#include "sapling.h"
#include "soil.h"

#include "climateconverter.h"
#include "csvfile.h"
#include "scriptglobal.h"
#include "mapgrid.h"

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
    // setup the scripting engine
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
    MapGridWrapper::addToScriptEngine(*mEngine);

    // default values for removal fractions
    // 100% of the stem, 0% of foliage and branches
    mRemoveFoliage = 0.;
    mRemoveBranch = 0.;
    mRemoveStem = 1.;


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
        qDebug() << "Script Error occured: " << mEngine->uncaughtException().toString() << "\n" << mEngine->uncaughtExceptionBacktrace();

}

int Management::remain(int number)
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
    return to_kill;
}


int Management::kill()
{
    int c = mTrees.count();
    for (int i=0;i<mTrees.count();i++)
        mTrees[i].first->remove();
    mTrees.clear();
    return c;
}

int Management::kill(QString filter, double fraction)
{
   return remove_trees(filter, fraction, false);
}
int Management::manage(QString filter, double fraction)
{
    return remove_trees(filter, fraction, true);
}

int Management::remove_percentiles(int pctfrom, int pctto, int number, bool management)
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
        if (management) {
            // management
            for (i=index_from; i<index_to; i++)
                mTrees.at(i).first->remove(removeFoliage(), removeBranch(), removeStem());
        } else {
            // just kill...
            for (i=index_from; i<index_to; i++)
                mTrees.at(i).first->remove();
        }
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
            if (management) {
                mTrees[rnd_index].first->remove( removeFoliage(), removeBranch(), removeStem() );
            } else {
                mTrees[rnd_index].first->remove();
            }
        }
    }
    qDebug() << count << "removed.";
    // clean up the tree list...
    for (int i=mTrees.count()-1; i>=0; --i) {
        if (mTrees[i].first->isDead())
            mTrees.removeAt(i);
    }
    return count; // killed or manages
}

/** remove trees from a list and reduce the list.

  */
int Management::remove_trees(QString expression, double fraction, bool management)
{
    TreeWrapper tw;
    Expression expr(expression,&tw);
    expr.enableIncSum();
    int n = 0;
    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    try {
        while (tp!=mTrees.end()) {
            tw.setTree(tp->first);
            // if expression evaluates to true and if random number below threshold...
            if (expr.calculate(tw) && drandom() <=fraction) {
                // remove from system
                if (management)
                    tp->first->remove(removeFoliage(), removeBranch(), removeStem()); // management with removal fractions
                else
                    tp->first->remove(); // kill
                // remove from tree list
                tp = mTrees.erase(tp);
                n++;
            } else {
                tp++;
            }
        }
    } catch(const IException &e) {
        context()->throwError(e.message());
    }
    return n;
}

// calculate aggregates for all trees in the internal list
double Management::aggregate_function(QString expression, QString filter, QString type)
{
    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    TreeWrapper tw;
    Expression expr(expression,&tw);

    double sum = 0.;
    int n=0;
    try {

        if (filter.isEmpty()) {
            // without filtering
            while (tp!=mTrees.end()) {
                tw.setTree(tp->first);
                sum += expr.calculate();
                ++n;
                ++tp;
            }
        } else {
            // with filtering
            Expression filter_expr(filter,&tw);
            filter_expr.enableIncSum();
            while (tp!=mTrees.end()) {
                tw.setTree(tp->first);
                if (filter_expr.calculate()) {
                    sum += expr.calculate();
                    ++n;
                }
                ++tp;
            }
        }

    } catch(const IException &e) {
        context()->throwError(e.message());
    }
    if (type=="sum")
        return sum;
    if (type=="mean")
        return n>0?sum/double(n):0.;
    return 0.;
}


// from the range percentile range pctfrom to pctto (each 1..100)
int Management::kill(int pctfrom, int pctto, int number)
{
    return remove_percentiles(pctfrom, pctto, number, false);
}

// from the range percentile range pctfrom to pctto (each 1..100)
int Management::manage(int pctfrom, int pctto, int number)
{
    return remove_percentiles(pctfrom, pctto, number, true);
}

int Management::manage()
{
    int c = mTrees.count();
    for (int i=0;i<mTrees.count();i++)
        mTrees[i].first->remove(removeFoliage(),
                                removeBranch(),
                                removeStem()); // remove with current removal fractions
    mTrees.clear();
    return c;
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
        qDebug() << "Script Error occured: " << mEngine->uncaughtException().toString() << "\n" << mEngine->uncaughtExceptionBacktrace();

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
    expr.enableIncSum();
    int n_before = mTrees.count();
    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    try {
        while (tp!=mTrees.end()) {
            tw.setTree(tp->first);
            if (expr.calculate(tw))
                tp = mTrees.erase(tp);
            else
                tp++;
        }
    } catch(const IException &e) {
        context()->throwError(e.message());
    }

    qDebug() << "filtering with" << filter << "N=" << n_before << "/" << mTrees.count()  << "trees (before/after filtering).";
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
        if (!ru->tree(i)->isDead())
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
        while (Tree *t=at.nextLiving())
            if (!t->isDead())
                mTrees.push_back(QPair<Tree*, double>(t, 0.));
    } else {
        Expression expr(filter,&tw);
        expr.enableIncSum();
        qDebug() << "filtering with" << filter;
        while (Tree *t=at.nextLiving()) {
            tw.setTree(t);
            if (!t->isDead() && expr.execute())
                mTrees.push_back(QPair<Tree*, double>(t, 0.));
        }
    }
    return mTrees.count();
}

/**
*/
void Management::loadFromTreeList(QList<Tree*>tree_list)
{
    mTrees.clear();
    for (int i=0;i<tree_list.count();++i)
        mTrees.append(QPair<Tree*, double>(tree_list[i], 0.));
}

// loadFromMap: script access
void Management::loadFromMap(MapGridWrapper *wrap, int key)
{
    if (!wrap) {
        context()->throwError("loadFromMap called with invalid map object!");
        return;
    }
    loadFromMap(wrap->map(), key);
}

void Management::killSaplings(MapGridWrapper *wrap, int key)
{
    //MapGridWrapper *wrap = qobject_cast<MapGridWrapper*>(map_grid_object.toQObject());
    //if (!wrap) {
    //    context()->throwError("loadFromMap called with invalid map object!");
    //    return;
    //}
    //loadFromMap(wrap->map(), key);
    // retrieve all sapling trees on the stand:
    QList<QPair<ResourceUnitSpecies *, SaplingTree *> > list = wrap->map()->saplingTrees(key);
    // for now, just kill em all...
    for (QList<QPair<ResourceUnitSpecies *, SaplingTree *> >::iterator it = list.begin(); it!=list.end(); ++it)
        (*it).second->pixel = 0;
    // the storage for unused/invalid saplingtrees is released lazily (once a year, after growth)
}

/// specify removal fractions
/// @param SWDFrac 0: no change, 1: remove all of standing woody debris
/// @param DWDfrac 0: no change, 1: remove all of downled woody debris
/// @param litterFrac 0: no change, 1: remove all of soil litter
/// @param soilFrac 0: no change, 1: remove all of soil organic matter
void Management::removeSoilCarbon(MapGridWrapper *wrap, int key, double SWDfrac, double DWDfrac, double litterFrac, double soilFrac)
{
    if (!(SWDfrac>=0. && SWDfrac<=1. && DWDfrac>=0. && DWDfrac<=1. && soilFrac>=0. && soilFrac<=1. && litterFrac>=0. && litterFrac<=1.)) {
        context()->throwError(QString("removeSoilCarbon called with invalid parameters!!\nArgs: %1").arg(context()->argumentsObject().toString()));
        return;
    }
    QList<QPair<ResourceUnit*, double> > ru_areas = wrap->map()->resourceUnitAreas(key);
    double total_area = 0.;
    for (int i=0;i<ru_areas.size();++i) {
        ResourceUnit *ru = ru_areas[i].first;
        double area_factor = ru_areas[i].second; // 0..1
        total_area += area_factor;
        // swd
        if (SWDfrac>0.)
            ru->snag()->removeCarbon(SWDfrac*area_factor);
        // soil pools
        ru->soil()->disturbance(DWDfrac*area_factor, litterFrac*area_factor, soilFrac*area_factor);
        // qDebug() << ru->index() << area_factor;
    }
    qDebug() << "total area" << total_area << "of" << wrap->map()->area(key);
}

/** slash snags (SWD and otherWood-Pools) of polygon \p key on the map \p wrap.
  The factor is scaled to the overlapping area of \p key on the resource unit.
  @param wrap MapGrid to use together with \p key
  @param key ID of the polygon.
  @param slash_fraction 0: no change, 1: 100%
   */
void Management::slashSnags(MapGridWrapper *wrap, int key, double slash_fraction)
{
    if (slash_fraction<0 || slash_fraction>1) {
        context()->throwError(QString("slashSnags called with invalid parameters!!\nArgs: %1").arg(context()->argumentsObject().toString()));
        return;
    }
    QList<QPair<ResourceUnit*, double> > ru_areas = wrap->map()->resourceUnitAreas(key);
    double total_area = 0.;
    for (int i=0;i<ru_areas.size();++i) {
        ResourceUnit *ru = ru_areas[i].first;
        double area_factor = ru_areas[i].second; // 0..1
        total_area += area_factor;
        ru->snag()->management(slash_fraction * area_factor);
        // qDebug() << ru->index() << area_factor;
    }
    qDebug() << "total area" << total_area << "of" << wrap->map()->area(key);

}

/** loadFromMap selects trees located on pixels with value 'key' within the grid 'map_grid'.
*/
void Management::loadFromMap(const MapGrid *map_grid, int key)
{
    if (!map_grid) {
        qDebug() << "invalid parameter for Management::loadFromMap: Map expected!";
        return;
    }
    if (map_grid->isValid()) {
        QList<Tree*> tree_list = map_grid->trees(key);
        loadFromTreeList( tree_list );
    } else {
        qDebug() << "Management::loadFromMap: grid is not valid - no trees loaded";
    }

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

/// random shuffle of all trees in the list
void Management::randomize()
{
    // fill the "value" part of the tree storage with a random value for each tree
    for (int i=0;i<mTrees.count(); ++i) {
        mTrees[i].second = drandom();
    }
    // now sort the list....
    qSort(mTrees.begin(), mTrees.end(), treePairValue);

}





