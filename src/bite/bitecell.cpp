#include "bitecell.h"
#include "globalsettings.h"
#include "model.h"
#include "grid.h"
#include "resourceunit.h"
#include "biteagent.h"
#include "fmtreelist.h"
#include "biteengine.h"
#include "bitelifecycle.h"
#include "biteclimate.h"

namespace BITE {

void BiteCell::setup(int cellidx, QPointF pos, BiteAgent *agent)
{
    mIndex = cellidx;
    mRU = GlobalSettings::instance()->model()->RUgrid().constValueAt(pos);
    mAgent = agent;
}

QString BiteCell::info()
{
    return QString("[%1 - %2]").arg(index()).arg(agent()->name());
}

void BiteCell::checkTreesLoaded(ABE::FMTreeList *treelist)
{
    if (!mTreesLoaded) {
        loadTrees(treelist);
        mTreesLoaded = true;
    }

}

double BiteCell::climateVar(int var_index) const
{
    // calculate the climate variable with 'var_index' - use the BiteClimate of the agent
    return mAgent->biteClimate().value(var_index, mRU);
}

void BiteCell::die()
{
    setActive(false);
    setSpreading(false);
    agent()->notifyItems(this, CellDied);
    mYearsLiving = 0;
}

void BiteCell::finalize()
{
    if (isActive()) {
        agent()->stats().nActive++;
        mYearsLiving++;

        // should the cell be active in the next iteration?
        setSpreading( agent()->lifeCycle()->shouldSpread(this) );
    }
}

void BiteCell::notify(ENotification what)
{
    switch (what) {
    case CellSpread:
        mLastSpread = BiteEngine::instance()->currentYear();
        break;
    default:
        break;
    }
}

int BiteCell::loadTrees(ABE::FMTreeList *treelist)
{
    Q_ASSERT(mRU != nullptr && mAgent != nullptr);

    QRectF rect = agent()->grid().cellRect( agent()->grid().indexOf(index()) );

    int added = treelist->loadFromRect(mRU, rect);
    return added;
}



} // end namespace
