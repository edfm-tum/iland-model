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
    mStandRect = ForestManagementEngine::instance()->standGrid()->boundingBox(mStand->id());
    mStandGrid.setup(mStandRect, cHeightSize);
    mStandOffset = ForestManagementEngine::instance()->standGrid()->grid().indexAt( mStandGrid.metricRect().topLeft() );
    // mask stand grid with actual stand (out of stand to -1)
    GridRunner<int> runner(ForestManagementEngine::instance()->standGrid()->grid(), mStandRect);
    short int *p=mStandGrid.begin();

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
    for (short int *p = mStandGrid.begin(); p!=mStandGrid.end(); ++p)
        *p = qMin(*p, ((short int)0));

    for(auto *p : mPatches) {
        for (auto idx : p->indices()) {
            mStandGrid[idx] = p->id();
        }
    }
}

int Patches::getPatch(QPoint position_lif)
{
    if (!GlobalSettings::instance()->model()->ABEngine()) return -1;
    int stand_id = ForestManagementEngine::instance()->standGrid()->standIDFromLIFCoord(position_lif);
    FMStand* stand = ForestManagementEngine::instance()->stand(stand_id);
    if (!stand) return -1;
    if (!stand->hasPatches()) return -1;
    return stand->patches()->patch(position_lif);
}

void Patches::createRandom(int n)
{
    int i=0;
    int found=0;
    while (++i < 10*n) {
        Patch *p=new Patch(this, mPatches.length() + 1);
        QPoint pt = mStandGrid.randomPosition();
        if (mStandGrid[pt]!=0)
            continue;
        p->indices().push_back(mStandGrid.index(pt));
        p->setRectangle(mStandGrid.cellRect(pt));
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

} // namespace ABE
