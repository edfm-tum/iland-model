#include "abe_global.h"

#include "patches.h"
#include "fmstand.h"
#include "forestmanagementengine.h"
#include "mapgrid.h"
#include "model.h"


namespace ABE {

Patches::Patches(QObject *parent)
    : QObject{parent}
{

}

void Patches::setup(FMStand *stand)
{
    mStand = stand;
    mStandRect = ForestManagementEngine::standGrid()->boundingBox(mStand->id());
    mLocalStandGrid.setup(mStandRect, cHeightSize);
    mStandOffset = ForestManagementEngine::standGrid()->grid().indexAt( mLocalStandGrid.metricRect().topLeft() );
    // mask stand grid with actual stand (out of stand to -1)
    GridRunner<int> runner(ForestManagementEngine::standGrid()->grid(), mStandRect);
    short int *p=mLocalStandGrid.begin();

    while (runner.next()) {
        if (*runner.current()!=mStand->id())
            *p = -1; // out of stand
        else
            *p = 0; // default
        ++p;
    }

}

void Patches::updateGrid()
{
    // reset to "no patch" = 0 (keep -1 flags)
    for (short int *p = mLocalStandGrid.begin(); p!=mLocalStandGrid.end(); ++p)
        *p = qMin(*p, ((short int)0));

    for(auto *p : mPatches) {
        p->update();
        for (auto idx : p->indices()) {
            mLocalStandGrid[idx] = p->id();
        }
    }
}

int Patches::getPatch(QPoint position_lif)
{
    if (!GlobalSettings::instance()->model()->ABEngine()) return -1;
    int stand_id = ForestManagementEngine::standGrid()->standIDFromLIFCoord(position_lif);
    FMStand* stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand) return -1;
    if (!stand->hasPatches()) return -1;
    return stand->patches()->patch(position_lif);
}

void Patches::createRandomPatches(int n)
{
    int i=0;
    int found=0;
    while (++i < 10*n) {
        Patch *p=new Patch(this, mPatches.length() + 1);
        QPoint pt = mLocalStandGrid.randomPosition();
        if (mLocalStandGrid[pt]!=0)
            continue;
        p->indices().push_back(mLocalStandGrid.index(pt));
        p->setRectangle(mLocalStandGrid.cellRect(pt));
        mPatches.push_back(p);
        if (++found >= n)
            break;
    }
    updateGrid();

}

void Patches::clear()
{
    qDeleteAll(mPatches);
    mPatches.clear();
}

bool Patches::createPatch(double x, double y, QString shape_string, int id)
{
    if (!ForestManagementEngine::standGrid()->grid().coordValid(x,y))
        return false;
    int s_id = ForestManagementEngine::standGrid()->grid().constValueAt(x,y);
    if (mStand->id() != s_id)
        return false;

}

void Patches::createStrips(double width, bool horizontal)
{
    clear();

    for (short int* pg= mLocalStandGrid.begin(); pg!=mLocalStandGrid.end(); ++pg) {
        if (*pg != -1) {
            QPointF p = mLocalStandGrid.cellCenterPoint(pg);
            double dx = p.x() - mLocalStandGrid.metricRect().left();
            double dy = p.y() - mLocalStandGrid.metricRect().top();
            int strip = (horizontal ? dy : dx) / width + 1;
            getPatch(strip, true)->indices().push_back(pg - mLocalStandGrid.begin());
        }
    }
    updateGrid();

}

bool Patches::createFromGrid(ScriptGrid *grid)
{
    clear();

    if (!grid || !grid->isCoordValid(mStandRect.x(), mStandRect.y()))
        return false;

    GridRunner<double> runner(grid->grid(), mStandRect);
    short int *pg=mLocalStandGrid.begin();

    while (runner.next()) {
        if (*pg == 0) {
            *pg = *runner.current();
            if (*pg > 0.) {
                Patch *pt = getPatch(*pg, true); // get / create patch
                pt->indices().push_back(pg - mLocalStandGrid.begin());
            }
        }
        ++pg;
    }
    updateGrid();
    return true;
}

Patch *Patches::getPatch(int patch_id, bool create_on_miss)
{
    for (auto *p : mPatches) {
        if (p->id() == patch_id)
            return p;
    }
    if (create_on_miss) {
        mPatches.push_back(new Patch(this, patch_id));
        return mPatches.last();
    } else {
        return nullptr;
    }
}

} // namespace ABE
