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
#include "global.h"
#include "amie_global.h"
#include "treelist.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "expressionwrapper.h"
#include "sapling.h"
#include "soil.h"
#include "scriptglobal.h"
#include "mapgrid.h"

namespace AMIE {

TreeList::TreeList(QObject *parent) :
    QObject(parent)
{
    // default values for removal fractions
    // 100% of the stem, 0% of foliage and branches
    mRemoveFoliage = 0.;
    mRemoveBranch = 0.;
    mRemoveStem = 1.;
}




int TreeList::remain(int number)
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


int TreeList::killAll()
{
    int c = mTrees.count();
    for (int i=0;i<mTrees.count();i++)
        mTrees[i].first->remove();
    mTrees.clear();
    return c;
}

int TreeList::kill(QString filter, double fraction)
{
   return remove_trees(filter, fraction, false);
}
int TreeList::manage(QString filter, double fraction)
{
    return remove_trees(filter, fraction, true);
}

int TreeList::remove_percentiles(int pctfrom, int pctto, int number, bool TreeList)
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
        if (TreeList) {
            // TreeList
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
                    qDebug() << "TreeList::kill: canceling search." << number << "trees left.";
                    count-=number; // not all trees were killed
                    break;
                }
                continue;
            }
            cancel = 1000;
            number--;
            if (TreeList) {
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
int TreeList::remove_trees(QString expression, double fraction, bool TreeList)
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
                if (TreeList)
                    tp->first->remove(removeFoliage(), removeBranch(), removeStem()); // TreeList with removal fractions
                else
                    tp->first->remove(); // kill
                // remove from tree list
                tp = mTrees.erase(tp);
                n++;
            } else {
                ++tp;
            }
        }
    } catch(const IException &e) {
        throwError(e.message());
    }
    return n;
}

// calculate aggregates for all trees in the internal list
double TreeList::aggregate_function(QString expression, QString filter, QString type)
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
        throwError(e.message());
    }
    if (type=="sum")
        return sum;
    if (type=="mean")
        return n>0?sum/double(n):0.;
    return 0.;
}

// introduced with switch to QJSEngine (context->throwMessage not available any more)
void TreeList::throwError(const QString &errormessage)
{
    GlobalSettings::instance()->scriptEngine()->evaluate(QString("throw '%1'").arg(errormessage));
    // no idea if this works!!!
}


// from the range percentile range pctfrom to pctto (each 1..100)
int TreeList::killPct(int pctfrom, int pctto, int number)
{
    return remove_percentiles(pctfrom, pctto, number, false);
}

// from the range percentile range pctfrom to pctto (each 1..100)
int TreeList::managePct(int pctfrom, int pctto, int number)
{
    return remove_percentiles(pctfrom, pctto, number, true);
}

int TreeList::manageAll()
{
    int c = mTrees.count();
    for (int i=0;i<mTrees.count();i++)
        mTrees[i].first->remove(removeFoliage(),
                                removeBranch(),
                                removeStem()); // remove with current removal fractions
    mTrees.clear();
    return c;
}



void TreeList::run()
{
    mRemoved=0;
    // todo: add this clearing the management main routine....
    if (mRemoved>0) {
        foreach(ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
           ru->cleanTreeList();
    }
}


int TreeList::filterIdList(QVariantList idList)
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
            ++tp;
    }
    qDebug() << "TreeList::filter by id-list:" << mTrees.count();
    return mTrees.count();
}

int TreeList::filter(QString filter)
{
    TreeWrapper tw;
    Expression expr(filter,&tw);
    expr.enableIncSum();
    int n_before = mTrees.count();
    QList<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    try {
        while (tp!=mTrees.end()) {
            tw.setTree(tp->first);
            double value = expr.calculate(tw);
            // keep if expression returns true (1)
            bool keep = value==1.;
            // if value is >0 (i.e. not "false"), then draw a random number
            if (!keep && value>0.)
                keep = drandom() < value;

            if (!keep)
                tp = mTrees.erase(tp);
            else
                ++tp;
        }
    } catch(const IException &e) {
        throwError(e.message());
    }

    qDebug() << "filtering with" << filter << "N=" << n_before << "/" << mTrees.count()  << "trees (before/after filtering).";
    return mTrees.count();
}

int TreeList::loadResourceUnit(int ruindex)
{
    Model *m = GlobalSettings::instance()->model();
    ResourceUnit *ru = m->ru(ruindex);
    if (!ru)
        return -1;
    mTrees.clear();
    mTrees.reserve(ru->trees().count());
    for (int i=0;i<ru->trees().count();i++)
        if (!ru->tree(i)->isDead())
            mTrees.push_back(QPair<Tree*,double>(ru->tree(i), 0.));
    return mTrees.count();
}

int TreeList::load(QString filter)
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
void TreeList::loadFromTreeList(QList<Tree*>tree_list)
{
    mTrees.clear();
    mTrees.reserve(tree_list.size());
    for (int i=0;i<tree_list.count();++i)
        mTrees.append(QPair<Tree*, double>(tree_list[i], 0.));
}

// loadFromMap: script access
void TreeList::loadFromMap(MapGridWrapper *wrap, int key)
{
    if (!wrap) {
        throwError("loadFromMap called with invalid map object!");
        return;
    }
    loadFromMap(wrap->map(), key);
}

void TreeList::killSaplings(MapGridWrapper *wrap, int key)
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
    for (QList<QPair<ResourceUnitSpecies *, SaplingTree *> >::iterator it = list.begin(); it!=list.end(); ++it) {
        // (*it).second->pixel = 0;
        (*it).first->changeSapling().clearSapling( *(*it).second, false); // kill and move biomass to soil
    }

    // the storage for unused/invalid saplingtrees is released lazily (once a year, after growth)
}

/// specify removal fractions
/// @param SWDFrac 0: no change, 1: remove all of standing woody debris
/// @param DWDfrac 0: no change, 1: remove all of downled woody debris
/// @param litterFrac 0: no change, 1: remove all of soil litter
/// @param soilFrac 0: no change, 1: remove all of soil organic matter
void TreeList::removeSoilCarbon(MapGridWrapper *wrap, int key, double SWDfrac, double DWDfrac, double litterFrac, double soilFrac)
{
    if (!(SWDfrac>=0. && SWDfrac<=1. && DWDfrac>=0. && DWDfrac<=1. && soilFrac>=0. && soilFrac<=1. && litterFrac>=0. && litterFrac<=1.)) {
        throwError(QString("removeSoilCarbon called with invalid parameters!!\nArgs: ---"));
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
void TreeList::slashSnags(MapGridWrapper *wrap, int key, double slash_fraction)
{
    if (slash_fraction<0 || slash_fraction>1) {
        throwError(QString("slashSnags called with invalid parameters!!\nArgs: ...."));
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
void TreeList::loadFromMap(const MapGrid *map_grid, int key)
{
    if (!map_grid) {
        qDebug() << "invalid parameter for TreeList::loadFromMap: Map expected!";
        return;
    }
    if (map_grid->isValid()) {
        QList<Tree*> tree_list = map_grid->trees(key);
        loadFromTreeList( tree_list );
    } else {
        qDebug() << "TreeList::loadFromMap: grid is not valid - no trees loaded";
    }

}

bool treePairValue(const QPair<Tree*, double> &p1, const QPair<Tree*, double> &p2)
{
    return p1.second < p2.second;
}

void TreeList::sort(QString statement)
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

double TreeList::percentile(int pct)
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
void TreeList::randomize()
{
    // fill the "value" part of the tree storage with a random value for each tree
    for (int i=0;i<mTrees.count(); ++i) {
        mTrees[i].second = drandom();
    }
    // now sort the list....
    qSort(mTrees.begin(), mTrees.end(), treePairValue);

}



} // namespace

