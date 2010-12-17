#include "mapgrid.h"
#include "globalsettings.h"
#include "model.h"
/** MapGrid encapsulates

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
        mRectIndex[*p]=mRectIndex[*p].united(mGrid.cellRect(mGrid.indexOf(p)));
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
