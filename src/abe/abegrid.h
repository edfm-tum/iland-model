#ifndef ABEGRID_H
#define ABEGRID_H
#include "layeredgrid.h"
namespace ABE {
class FMStand; // forward
class Agent; // forward
}

/** Helper class for visualizing ABE management data.
*/
typedef ABE::FMStand* FMStandPtr;
class ABELayers: public LayeredGrid<FMStandPtr> {
  public:
    ~ABELayers();
    void setGrid(Grid<FMStandPtr> &grid) { mGrid = &grid; }
    double value(const FMStandPtr &data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> names() const;
    const QString labelvalue(const int value, const int index) const;
    void registerLayers();
    void clearClasses(); // clear ID and agent classes...
private:
    mutable QHash<const ABE::Agent*, int > mAgentIndex;
    mutable QHash<QString, int> mUnitIndex;
    mutable QHash<int, int> mStandIndex;
};



#endif // ABEGRID_H
