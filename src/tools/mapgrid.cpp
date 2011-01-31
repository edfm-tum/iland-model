#include "mapgrid.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "grid.h"
#include "sapling.h"
#include "resourceunit.h"
/** MapGrid encapsulates maps that classify the area in 10m resolution (e.g. for stand-types, management-plans, ...)
  The grid is (currently) loaded from disk in a ESRI style text file format. See also the "location" keys and GisTransformation classes for
  details on how the grid is mapped to the local coordinate system of the project area. From the source grid a 10m grid
  using the extent and position of the "HeightGrid" and spatial indices for faster access are generated.
  The grid is clipped to the extent of the simulation area and -1 is used for no_data_values.
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

    const QRectF &world = GlobalSettings::instance()->model()->extent();
    QPointF p;
    for (int i=0;i<mGrid.count();i++) {
        p = mGrid.cellCenterPoint(mGrid.indexOf(i));
        if (source_grid.value(p) != source_grid.noDataValue() && world.contains(p) )
            mGrid.valueAtIndex(i) = source_grid.value(p);
        else
            mGrid.valueAtIndex(i) = -1;
    }

    // create spatial index
    mRectIndex.clear();
    mRUIndex.clear();

    for (int *p = mGrid.begin(); p!=mGrid.end(); ++p) {
        if (*p==-1)
            continue;
        QPair<QRectF,double> &data = mRectIndex[*p];
        data.first = data.first.united(mGrid.cellRect(mGrid.indexOf(p)));
        data.second += cPxSize*cPxPerHeight*cPxSize*cPxPerHeight; // 100m2

        ResourceUnit *ru = GlobalSettings::instance()->model()->ru(mGrid.cellCenterPoint(mGrid.indexOf(p)));
        // find all entries for the current grid id
        QMultiHash<int, QPair<ResourceUnit*, double> >::iterator pos = mRUIndex.find(*p);

        // look for the resource unit 'ru'
        bool found = false;
        while (pos!=mRUIndex.end() && pos.key() == *p) {
            if (pos.value().first == ru) {
                pos.value().second+= 0.01; // 1 pixel = 1% of the area
                found=true;
                break;
            }
            ++pos;
        }
        if (!found)
            mRUIndex.insertMulti(*p, QPair<ResourceUnit*, double>(ru, 0.01));

    }
    return true;

}

bool MapGrid::loadFromFile(const QString &fileName)
{
    GisGrid gis_grid;
    mName = "invalid";
    if (gis_grid.loadFromFile(fileName)) {
        mName = fileName;
        return loadFromGrid(gis_grid);
    }
    return false;
}

/// returns the list of resource units with at least one pixel within the area designated by 'id'
QList<ResourceUnit *> MapGrid::resourceUnits(const int id) const
{
    QList<ResourceUnit *> result;
    QList<QPair<ResourceUnit*, double> > list = mRUIndex.values();
    for (int i=0;i<list.count();++i)
        result.append( list[i].first);
    return result;
}

/// return a list of all trees on the area denoted by 'id'
QList<Tree *> MapGrid::trees(const int id) const
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

/// return a list of grid-indices of a given stand-id
/// The selection is limited to pixels within the world's extent
QList<int> MapGrid::gridIndices(const int id) const
{
    QList<int> result;
    QRectF rect = mRectIndex[id].first;
    GridRunner<int> run(mGrid, rect);
    while (int *cell = run.next()) {
       if (*cell == id)
         result.push_back(cell - mGrid.begin());
    }
    return result;
}

QList<QPair<ResourceUnitSpecies *, SaplingTree *> > MapGrid::saplingTrees(const int id) const
{
    QList<QPair<ResourceUnitSpecies *, SaplingTree *> > result;
    QList<ResourceUnit*> resource_units = resourceUnits(id);
    foreach(ResourceUnit *ru, resource_units) {
        foreach(ResourceUnitSpecies *rus, ru->ruSpecies()) {
            foreach(const SaplingTree &tree, rus->sapling().saplings()) {
                if (gridValue( GlobalSettings::instance()->model()->grid()->indexOf(tree.pixel) ) == id)
                    result.push_back( QPair<ResourceUnitSpecies *, SaplingTree *>(rus, &const_cast<SaplingTree&>(tree)) );
            }
        }
    }
    qDebug() << "loaded" << result.count() << "sapling trees";
    return result;

}


