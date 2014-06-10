#include "global.h"
#include "abe_global.h"
#include "fmtreelist.h"

#include "forestmanagementengine.h"
// iLand stuff
#include "tree.h"
#include "expression.h"
#include "mapgrid.h"
#include "expressionwrapper.h"
#include "model.h"
#include "helper.h"
#include "fmstand.h"
#include "fomescript.h"

namespace ABE {

// TODO: fix: removal fractions need to be moved to agent/units/ whatever....
double removeFoliage()  {return 0.;}
double removeStem()  {return 1.;}
double removeBranch()  {return 0.;}

FMTreeList::FMTreeList(QObject *parent) :
    QObject(parent)
{
    setStand(0); // clear stand link

}

FMTreeList::FMTreeList(FMStand *stand, QObject *parent):
    QObject(parent)
{
    setStand(stand);
}

void FMTreeList::setStand(FMStand *stand)
{
    mStand = stand;
    if (stand) {
        mStandId = stand->id();
        mNumberOfStems = stand->stems() * stand->area();
        mOnlySimulate = stand->currentActivity()?stand->currentFlags().isScheduled() : false;
        mStandRect=QRectF();
    } else {
        mStandId = -1;
        mNumberOfStems = 1000;
        mOnlySimulate = false;
    }
}



int FMTreeList::load(const QString &filter)
{
    if (standId()>-1) {
        // load all trees of the current stand
        const MapGrid *map = ForestManagementEngine::instance()->standGrid();
        if (map->isValid()) {
            map->loadTrees(mStandId, mTrees, filter, mNumberOfStems);
        } else {
            qCDebug(abe) << "FMTreeList::load: grid is not valid - no trees loaded";
        }
        return mTrees.count();

    } else {
        qCDebug(abe) << "FMTreeList::load: loading *all* trees, because stand id is -1";
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
}

int FMTreeList::removeMarkedTrees()
{
    loadAll();
    int n_removed = 0;
    for (QVector<QPair<Tree*, double> >::const_iterator it = mTrees.constBegin(); it!=mTrees.constEnd(); ++it) {
        Tree *t = const_cast<Tree*>((*it).first);
        if (t->isMarkedForCut()) {
            t->remove();
            n_removed++;
        } else if (t->isMarkedForHarvest()) {
            t->remove(removeFoliage(), removeBranch(), removeStem());
            n_removed++;
        }
    }
    return n_removed;
    if (mStand->trace())
        qCDebug(abe) << mStand->context() << "removeMarkedTrees: n=" << n_removed;
}

int FMTreeList::harvest(QString filter, double fraction)
{
    return remove_trees(filter, fraction, true);

}

bool FMTreeList::trace() const
{
    return FomeScript::bridge()->standObj()->trace();
}


int FMTreeList::remove_percentiles(int pctfrom, int pctto, int number, bool management)
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
                if (simulate()) {
                    mTrees.at(i).first->markForHarvest(true);
                    mStand->addScheduledHarvest(mTrees.at(i).first->volume());
                } else {
                    mTrees.at(i).first->remove(removeFoliage(), removeBranch(), removeStem());
                }
        } else {
            // just kill...
            for (i=index_from; i<index_to; i++)
                if (simulate()) {
                    mTrees.at(i).first->markForCut(true);
                    mStand->addScheduledHarvest(mTrees.at(i).first->volume());
                } else
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
                if (simulate()) {
                    mTrees[rnd_index].first->markForHarvest(true);
                    mStand->addScheduledHarvest( mTrees[rnd_index].first->volume());
                } else
                    mTrees[rnd_index].first->remove( removeFoliage(), removeBranch(), removeStem() );
            } else {
                if (simulate()) {
                    mTrees[rnd_index].first->markForCut(true);
                    mStand->addScheduledHarvest( mTrees[rnd_index].first->volume());
                } else
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
int FMTreeList::remove_trees(QString expression, double fraction, bool management)
{
    TreeWrapper tw;
    if (expression.isEmpty())
        expression="true";
    Expression expr(expression,&tw);
    expr.enableIncSum();
    int n = 0;
    QVector<QPair<Tree*, double> >::iterator tp=mTrees.begin();
    try {
        while (tp!=mTrees.end()) {
            tw.setTree(tp->first);
            // if expression evaluates to true and if random number below threshold...
            if (expr.calculate(tw) && drandom() <=fraction) {
                // remove from system
                if (management) {
                    if (simulate()) {
                        tp->first->markForHarvest(true);
                        mStand->addScheduledHarvest(tp->first->volume());
                    } else
                        tp->first->remove(removeFoliage(), removeBranch(), removeStem()); // management with removal fractions
                } else {
                    if (simulate()) {
                        tp->first->markForCut(true);
                        mStand->addScheduledHarvest(tp->first->volume());
                    } else
                        tp->first->remove(); // kill
                }
                // remove from tree list
                tp = mTrees.erase(tp);
                n++;
            } else {
                ++tp;
            }
        }
    } catch(const IException &e) {
        qCWarning(abe) << "treelist: remove_trees: expression:" << expression << ", msg:" << e.message();
    }
    return n;

}

double FMTreeList::aggregate_function(QString expression, QString filter, QString type)
{
    QVector<QPair<Tree*, double> >::iterator tp=mTrees.begin();
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
        qCWarning(abe) << "Treelist: aggregate function: expression:" << expression << ", filter:" << filter << ", msg:" <<e.message();
        //throwError(e.message());
    }
    if (type=="sum")
        return sum;
    if (type=="mean")
        return n>0?sum/double(n):0.;
    return 0.;

}

void FMTreeList::prepareGrids()
{
    QRectF box = ForestManagementEngine::instance()->standGrid()->boundingBox(mStand->id());
    if (mStandRect==box)
        return;
    mStandRect = box;
    // the memory of the grids is only reallocated if the current box is larger then the previous...
    mStandGrid.setup(box, cHeightSize);
    mTreeCountGrid.setup(box, cHeightSize);
    // mark areas outside of the grid...
    GridRunner<int> runner(ForestManagementEngine::instance()->standGrid()->grid(), box);
    float *p=mStandGrid.begin();
    while (runner.next()) {
        if (*runner.current()!=mStand->id())
            *p=-1.f;
        ++p;
    }
}

void FMTreeList::runGrid(void (*func)(float &, int &, const Tree *, const FMTreeList *))
{
    if (mStandRect.isNull())
        prepareGrids();

    mStandGrid.initialize(0.f);
    mTreeCountGrid.initialize(0);
    for (QVector<QPair<Tree*, double> >::const_iterator it=mTrees.constBegin(); it!=mTrees.constEnd(); ++it) {
        const Tree* tree = it->first;
        QPoint p = mStandGrid.indexAt(tree->position());
        (*func)(mStandGrid.valueAtIndex(p), mTreeCountGrid.valueAtIndex(p), tree, this);
    }

    // finalization: call again for each *cell*
    for (int i=0;i<mStandGrid.count();++i)
        (*func)(mStandGrid.valueAtIndex(i), mTreeCountGrid.valueAtIndex(i), 0, this);

}

void rungrid_heightmax(float &cell, int &n, const Tree *tree, const FMTreeList *list)
{
    Q_UNUSED(n); Q_UNUSED(list);
    if (tree)
        cell = qMax(cell, tree->height());
}
void rungrid_basalarea(float &cell, int &n, const Tree *tree, const FMTreeList *list)
{
    Q_UNUSED(list);
    if (tree) {
        cell += tree->basalArea();
        ++n;
    } else {
        if (n>0)
            cell /= float(n);
    }
}
void rungrid_volume(float &cell, int &n, const Tree *tree, const FMTreeList *list)
{
    Q_UNUSED(list);
    if (tree) {
        cell += tree->volume();
        ++n;
    } else {
        if (n>0)
            cell /= float(n);
    }
}

void rungrid_custom(float &cell, int &n, const Tree *tree, const FMTreeList *list)
{
    if (tree) {
        *list->mRunGridCustomCell = cell;
        TreeWrapper tw(tree);
        cell = list->mRunGridCustom->calculate(tw);
        ++n;
    }
}
void FMTreeList::prepareStandGrid(QString type, QString custom_expression)
{
    if(!mStand){
        qCDebug(abe) << "Error: FMTreeList: no current stand defined.";
        return;
    }

    if (type==QStringLiteral("height")) {
        return runGrid(&rungrid_heightmax);
    }

    if (type==QStringLiteral("basalArea"))
        return runGrid(&rungrid_basalarea);

    if (type==QStringLiteral("volume"))
        return runGrid(&rungrid_volume);

    if (type==QStringLiteral("custom")) {
        mRunGridCustom = new Expression(custom_expression);
        mRunGridCustomCell = mRunGridCustom->addVar("cell");
        runGrid(&rungrid_custom);
        delete mRunGridCustom;
        mRunGridCustom = 0;
        return;
    }
    qCDebug(abe) << "FMTreeList: invalid type for prepareStandGrid: " << type;
}

void FMTreeList::exportStandGrid(QString file_name)
{
    file_name = GlobalSettings::instance()->path(file_name);
    Helper::saveToTextFile(file_name, gridToESRIRaster(mStandGrid) );
    qCDebug(abe) << "saved grid to file" << file_name;
}
} // namespace