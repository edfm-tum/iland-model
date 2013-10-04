/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "mapgrid.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "grid.h"
#include "sapling.h"
#include "resourceunit.h"
/** MapGrid encapsulates maps that classify the area in 10m resolution (e.g. for stand-types, management-plans, ...)
  @ingroup tools
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

bool MapGrid::loadFromGrid(const GisGrid &source_grid, const bool create_index)
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

    if (create_index) {

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
    }
    return true;

}

bool MapGrid::loadFromFile(const QString &fileName, const bool create_index)
{
    GisGrid gis_grid;
    mName = "invalid";
    if (gis_grid.loadFromFile(fileName)) {
        mName = fileName;
        return loadFromGrid(gis_grid, create_index);
    }
    return false;
}

/// returns the list of resource units with at least one pixel within the area designated by 'id'???? ID UNUSED???
QList<ResourceUnit *> MapGrid::resourceUnits(const int id) const
{
    QList<ResourceUnit *> result;
    QList<QPair<ResourceUnit*, double> > list = mRUIndex.values();
    for (int i=0;i<list.count();++i)
        result.append( list[i].first);
    return result;
}

/// return a list of all living trees on the area denoted by 'id'
QList<Tree *> MapGrid::trees(const int id) const
{
    QList<Tree*> tree_list;
    QList<ResourceUnit*> resource_units = resourceUnits(id);
    foreach(ResourceUnit *ru, resource_units) {
        foreach(const Tree &tree, ru->constTrees())
            if (gridValue(tree.positionIndex()) == id && !tree.isDead()) {
                tree_list.append( & const_cast<Tree&>(tree) );
            }
    }
//    qDebug() << "MapGrid::trees: found" << c << "/" << tree_list.size();
    return tree_list;

}

/// return a list of grid-indices of a given stand-id (a grid-index
/// is the index of 10m x 10m pixels within the internal storage)
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

/// retrieve a list of all stands that are neighbors of the stand with ID "index".
QList<int> MapGrid::neighborsOf(const int index) const
{
    if (mNeighborList.isEmpty())
        const_cast<MapGrid*>(this)->fillNeighborList(); // fill the list
    return mNeighborList.values(index);
}

/// scan the map and add neighborhood-relations to the mNeighborList
/// the 4-neighborhood is used to identify neighbors.
void MapGrid::fillNeighborList()
{
    mNeighborList.clear();
    GridRunner<int> gr(mGrid, mGrid.rectangle()); //  the full grid
    int *n4[4];
    QHash<int,int>::iterator it_hash;
    while (gr.next()) {
        gr.neighbors4(n4); // get the four-neighborhood (0-pointers possible)
        for (int i=0;i<4;++i)
            if (n4[i] && *gr.current() != *n4[i]) {
                // look if we already have the pair
                it_hash = mNeighborList.find(*gr.current(), *n4[i]);
                if (it_hash == mNeighborList.end()) {
                    // add the "edge" two times in the hash
                    mNeighborList.insertMulti(*gr.current(), *n4[i]);
                    mNeighborList.insertMulti(*n4[i], *gr.current());
                }
            }
    }

}


