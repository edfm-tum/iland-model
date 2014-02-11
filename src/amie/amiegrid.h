#ifndef AMIEGRID_H
#define AMIEGRID_H
#include "layeredgrid.h"
namespace AMIE {
class FMStand; // forward
}


/** Helper class for visualizing AMIE management data.
*/
typedef AMIE::FMStand* FMStandPtr;
class AMIELayers: public LayeredGrid<FMStandPtr> {
  public:
    void setGrid(Grid<FMStandPtr> &grid) { mGrid = &grid; }
    double value(const FMStandPtr &data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> names() const;
    const QString labelvalue(const int value, const int index) const;
    void registerLayers();
    void clearClasses(); // clear ID and agent classes...
private:
    QHash<QString, int> mAgentIndex;
    mutable QHash<QString, int> mUnitIndex;
    mutable QHash<int, int> mStandIndex;
};

#endif // AMIEGRID_H
