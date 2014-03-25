#include "global.h"
#include "amie_global.h"
#include "fmtreelist.h"

#include "forestmanagementengine.h"
// iLand stuff
#include "tree.h"
#include "expression.h"
#include "mapgrid.h"

namespace AMIE {

FMTreeList::FMTreeList(QObject *parent) :
    QObject(parent)
{
    mStandId = -1;
}

int FMTreeList::load(const QString &filter)
{
    if (standId()>-1) {
        // load all trees of the current stand
        const MapGrid *map = ForestManagementEngine::instance()->standGrid();
        if (map->isValid()) {
            map->loadTrees(mStandId, mTrees, filter);
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


int FMTreeList::load(QString filter)
{
}


} // namespace
