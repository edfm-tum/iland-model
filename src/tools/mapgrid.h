#ifndef MAPGRID_H
#define MAPGRID_H
#include <QtCore/QHash>
#include <QRectF>
#include "grid.h"
#include "gisgrid.h"
class ResourceUnit; // forward
class Tree; // forward
class SaplingTree; // forward
class ResourceUnitSpecies; // forward

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
    const Grid<int> &grid() const { return mGrid; }
    // access
    QRectF boundingBox(const int id) const { return mRectIndex[id].first; } ///< returns the bounding box of a polygon
    double area(const int id) const {return mRectIndex[id].second;} ///< return the area (m2) covered by the polygon
    QList<ResourceUnit*> resourceUnits(const int id) const { return mRUIndex.values(id);} ///< returns the list of resource units with at least one pixel within the area designated by 'id'
    /// return a list of all trees on the area 'id'
    QList<Tree*> trees(const int id) const;
    /// return a list of grid-indices of a given stand-id
    QList<int> gridIndices(const int id) const;
    /// get a list of sapling trees on a given stand.
    QList<QPair<ResourceUnitSpecies *, SaplingTree *> > saplingTrees(const int id) const;
    /// return true, if the point 'lif_grid_coords' (x/y integer key within the LIF-Grid)
    inline bool hasValue(const int id, const QPoint &lif_grid_coords) const { return mGrid.constValueAtIndex(lif_grid_coords.x()/cPxPerHeight, lif_grid_coords.y()/cPxPerHeight) == id; }
    inline int gridValue(const QPoint &lif_grid_coords) const  { return mGrid.constValueAtIndex(lif_grid_coords.x()/cPxPerHeight, lif_grid_coords.y()/cPxPerHeight); }
private:
    Grid<int> mGrid;
    QHash<int, QPair<QRectF,double> > mRectIndex; ///< holds the extent and area for each map-id
    QMultiHash<int, ResourceUnit*> mRUIndex; ///< holds a list of resource units per map-id
};

#endif // MAPGRID_H
