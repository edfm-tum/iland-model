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
        mCumYearsLiving++;

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
