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
    const QStringList names() const;
    void registerLayers();
private:
};

#endif // AMIEGRID_H
