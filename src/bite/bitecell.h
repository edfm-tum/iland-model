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
    BiteCell() : mRU(nullptr), mIsActive(false), mIsSpreading(false), mIndex(-1), mTreesLoaded(false), mYearsLiving(0) {}
    void setup(int cellidx, QPointF pos, BiteAgent *agent);
    /// index within the agent grid
    int index() const {return mIndex;}
    BiteAgent *agent() const { return mAgent; }

    bool isActive() const {return mIsActive; }
    void setActive(bool activate) { mIsActive = activate; }

    bool isSpreading() const {return mIsSpreading; }
    void setSpreading(bool activate) { mIsSpreading = activate; }

    void setTreesLoaded(bool loaded) { mTreesLoaded = loaded; }
    void checkTreesLoaded(ABE::FMTreeList *treelist);

    int yearsLiving() const { return mYearsLiving; }
    int yearLastSpread() const { return mLastSpread; }

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
};

} // end namespace
#endif // BCELL_H
