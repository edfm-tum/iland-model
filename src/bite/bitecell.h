#ifndef BCELL_H
#define BCELL_H

class ResourceUnit;
class Tree;
#include <QPointF>
namespace ABE {
class FMTreeList; // forward
}

namespace BITE {

class BiteAgent;

class BiteCell
{
public:
    BiteCell() : mRU(nullptr), mIsActive(false) {}
    void setup(int cellidx, QPointF pos, BiteAgent *agent);
    /// index within the agent grid
    int index() const {return mIndex;}
    BiteAgent *agent() const { return mAgent; }

    bool isActive() const {return mIsActive; }
    void setActive(bool activate) { mIsActive = activate; }

    int loadTrees(ABE::FMTreeList *treelist);
private:
    ResourceUnit *mRU; ///< ptr to resource unit of the cell
    BiteAgent *mAgent; ///< link to the agent
    bool mIsActive;
    int mIndex; // index within the grid
};

} // end namespace
#endif // BCELL_H
