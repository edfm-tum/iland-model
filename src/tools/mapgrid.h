#ifndef MAPGRID_H
#define MAPGRID_H
#include <QtCore/QHash>
#include <QRectF>
#include "grid.h"
#include "gisgrid.h"
class ResourceUnit; // forward

class MapGrid
{
public:
    MapGrid();
    MapGrid(const GisGrid &source_grid) { loadFromGrid(source_grid); }
    MapGrid(const QString &fileName) { loadFromFile(fileName); }
    bool loadFromFile(const QString &fileName); ///< load ESRI style text file
    bool loadFromGrid(const GisGrid &source_grid); ///< load from an already present GisGrid
    // access
    bool isValid() const { return !mGrid.isEmpty(); }
    Grid<int> &grid() { return mGrid; }
    // access
    QRectF boundingBox(const int id) { return mRectIndex[id]; }
    QList<ResourceUnit*> resourceUnits(const int id) { return mRUIndex.values(id);}
private:
    Grid<int> mGrid;
    QHash<int, QRectF> mRectIndex; ///< holds the extent for each map-id
    QMultiHash<int, ResourceUnit*> mRUIndex; ///< holds a list of resource units per map-id
};

#endif // MAPGRID_H
