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
#ifndef BCELL_H
#define BCELL_H

class ResourceUnit;
class Tree;
#include <QPointF>

#include "biteclimate.h"

namespace ABE {
class FMTreeList; // forward
}

namespace BITE {

class BiteAgent;

class BiteCell
{
public:
    BiteCell() : mRU(nullptr), mIsActive(false), mIsSpreading(false), mIndex(-1), mTreesLoaded(false), mYearsLiving(0), mCumYearsLiving(0) {}
    void setup(int cellidx, QPointF pos, BiteAgent *agent);
    /// index within the agent grid
    int index() const {return mIndex;}
    BiteAgent *agent() const { return mAgent; }
    QString info();

    bool isActive() const {return mIsActive; }
    void setActive(bool activate) { mIsActive = activate; }

    bool isSpreading() const {return mIsSpreading; }
    void setSpreading(bool activate) { mIsSpreading = activate; }

    void setTreesLoaded(bool loaded) { mTreesLoaded = loaded; }
    void checkTreesLoaded(ABE::FMTreeList *treelist);

    int yearsLiving() const { return mYearsLiving; }
    int yearLastSpread() const { return mLastSpread; }
    int cumYearsLiving() const { return mCumYearsLiving; }

    // climate vars
    double climateVar(int var_index) const;
    // actions
    void die();
    void finalize();
    enum ENotification { CellDied, CellColonized, CellSpread, CellImpacted };
    void notify(ENotification what);

    int loadTrees(ABE::FMTreeList *treelist);
private:
    ResourceUnit *mRU; ///< ptr to resource unit of the cell
    BiteAgent *mAgent; ///< link to the agent
    bool mIsActive; ///< active: true if the agent "lives" on the cell
    bool mIsSpreading; ///< true: the agent spreads from the cell
    int mIndex; ///< index within the grid
    bool mTreesLoaded; ///< is the tree list already fetched from iLand
    int mYearsLiving; ///< years a cell is already active/living
    int mLastSpread; ///< year of last spread
    int mCumYearsLiving; ///< cumulative number of years a cell is active
};

} // end namespace
#endif // BCELL_H
