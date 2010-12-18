#include "mapgrid.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
/** MapGrid encapsulates maps that classify the area in 10m resolution (e.g. for stand-types, management-plans, ...)
  The grid is (currently) loaded from disk in a ESRI style text file format. See also the "location" keys and GisTransformation classes for
  details on how the grid is mapped to the local coordinate system of the project area. From the source grid a 10m grid
  using the extent and position of the "HeightGrid" and spatial indices for faster access are generated.
  Use boundingBox(), resourceUnits(), trees() to retrieve information for specific 'ids'. gridValue() retrieves the 'id' for a given
  location (in LIF-coordinates).

  */
MapGrid::MapGrid()
{
}

bool MapGrid::loadFromGrid(const GisGrid &source_grid)
{
    if (!GlobalSettings::instance()->model())
        throw IException("GisGrid::create10mGrid: no valid model to retrieve height grid.");

    HeightGrid *h_grid = GlobalSettings::instance()->model()->heightGrid();
    if (!h_grid || h_grid->isEmpty())
        throw IException("GisGrid::create10mGrid: no valid height grid to copy grid size.");
    // create a grid with the same size as the height grid
    // (height-grid: 10m size, covering the full extent)
    mGrid.clear();
    mGrid.setup(h_grid->metricRect(),h_grid->cellsize());

    for (int i=0;i<mGrid.count();i++)
        mGrid.valueAtIndex(i) = source_grid.value(mGrid.cellCenterPoint(mGrid.indexOf(i)));

    // create spatial index
    mRectIndex.clear();
    mRUIndex.clear();

    for (int *p = mGrid.begin(); p!=mGrid.end(); ++p) {
        QPair<QRectF,double> &data = mRectIndex[*p];
        data.first = data.first.united(mGrid.cellRect(mGrid.indexOf(p)));
        data.second += cPxSize*cPxPerHeight*cPxSize*cPxPerHeight; // 100m2
        //mRectIndex[*p]=mRectIndex[*p].united(mGrid.cellRect(mGrid.indexOf(p)));
        ResourceUnit *ru = GlobalSettings::instance()->model()->ru(mGrid.cellCenterPoint(mGrid.indexOf(p)));
        if (!mRUIndex.contains(*p, ru))
            mRUIndex.insertMulti(*p, ru);
    }
    return true;

}

bool MapGrid::loadFromFile(const QString &fileName)
{
    GisGrid gis_grid;
    if (gis_grid.loadFromFile(fileName)) {
        return loadFromGrid(gis_grid);
    }
    return false;
}

/// return a list of all trees on the area denoted by 'id'
QList<Tree *> MapGrid::trees(const int id)
{
    QList<Tree*> tree_list;
    QList<ResourceUnit*> resource_units = resourceUnits(id);
    foreach(ResourceUnit *ru, resource_units) {
        foreach(const Tree &tree, ru->constTrees())
            if (gridValue(tree.positionIndex()) == id)
                tree_list.append( & const_cast<Tree&>(tree) );
    }
    return tree_list;

}
