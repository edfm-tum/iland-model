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
#include "global.h"
#include "tree.h"

#include "grid.h"

#include "stamp.h"
#include "species.h"
#include "resourceunit.h"
#include "model.h"
#include "snag.h"

#include "forestmanagementengine.h"

// static varaibles
FloatGrid *Tree::mGrid = 0;
HeightGrid *Tree::mHeightGrid = 0;
int Tree::m_statPrint=0;
int Tree::m_statAboveZ=0;
int Tree::m_statCreated=0;
int Tree::m_nextId=0;

/** @class Tree
    @ingroup core
    A tree is the basic simulation entity of iLand and represents a single tree.
    Trees in iLand are designed to be lightweight, thus the list of stored properties is limited. Basic properties
    are dimensions (dbh, height), biomass pools (stem, leaves, roots), the reserve NPP pool. Additionally, the location and species are stored.
    A Tree has a height of at least 4m; trees below this threshold are covered by the regeneration layer (see Sapling).
    Trees are stored in lists managed at the resource unit level.

  */

/** get distance and direction between two points.
  returns the distance (m), and the angle between PStart and PEnd (radians) in referenced param rAngle. */
float dist_and_direction(const QPointF &PStart, const QPointF &PEnd, float &rAngle)
{
    float dx = PEnd.x() - PStart.x();
    float dy = PEnd.y() - PStart.y();
    float d = sqrt(dx*dx + dy*dy);
    // direction:
    rAngle = atan2(dx, dy);
    return d;
}


// lifecycle
Tree::Tree()
{
    mDbh = mHeight = 0;
    mRU = 0; mSpecies = 0;
    mFlags = mAge = 0;
    mOpacity=mFoliageMass=mWoodyMass=mCoarseRootMass=mFineRootMass=mLeafArea=0.;
    mDbhDelta=mNPPReserve=mLRI=mStressIndex=0.;
    mLightResponse = 0.;
    mStamp=0;
    mId = m_nextId++;
    m_statCreated++;
}

float Tree::crownRadius() const
{
    Q_ASSERT(mStamp!=0);
    return mStamp->crownRadius();
}

float Tree::biomassBranch() const
{
    return mSpecies->biomassBranch(mDbh);
}

void Tree::setGrid(FloatGrid* gridToStamp, Grid<HeightGridValue> *dominanceGrid)
{
    mGrid = gridToStamp; mHeightGrid = dominanceGrid;
}

// calculate the thickness of the bark of the tree
double Tree::barkThickness() const
{
    return mSpecies->barkThickness(mDbh);
}

/// dumps some core variables of a tree to a string.
QString Tree::dump()
{
    QString result = QString("id %1 species %2 dbh %3 h %4 x/y %5/%6 ru# %7 LRI %8")
                            .arg(mId).arg(species()->id()).arg(mDbh).arg(mHeight)
                            .arg(position().x()).arg(position().y())
                            .arg(mRU->index()).arg(mLRI);
    return result;
}

void Tree::dumpList(DebugList &rTargetList)
{
    rTargetList << mId << species()->id() << mDbh << mHeight  << position().x() << position().y()   << mRU->index() << mLRI
                << mWoodyMass << mCoarseRootMass << mFoliageMass << mLeafArea;
}

void Tree::setup()
{
    if (mDbh<=0 || mHeight<=0) {
        throw IException(QString("Error: trying to set up a tree with invalid dimensions: dbh: %1 height: %2 id: %3 RU-index: %4").arg(mDbh).arg(mHeight).arg(mId).arg(mRU->index()));
    }
    // check stamp
    Q_ASSERT_X(species()!=0, "Tree::setup()", "species is NULL");
    mStamp = species()->stamp(mDbh, mHeight);
    if (!mStamp) {
        throw IException("Tree::setup() with invalid stamp!");
    }

    mFoliageMass = species()->biomassFoliage(mDbh);
    mCoarseRootMass = species()->biomassRoot(mDbh); // coarse root (allometry)
    mFineRootMass = mFoliageMass * species()->finerootFoliageRatio(); //  fine root (size defined  by finerootFoliageRatio)
    mWoodyMass = species()->biomassWoody(mDbh);

    // LeafArea[m2] = LeafMass[kg] * specificLeafArea[m2/kg]
    mLeafArea = mFoliageMass * species()->specificLeafArea();
    mOpacity = 1. - exp(- Model::settings().lightExtinctionCoefficientOpacity * mLeafArea / mStamp->crownArea());
    mNPPReserve = (1+species()->finerootFoliageRatio())*mFoliageMass; // initial value
    mDbhDelta = 0.1f; // initial value: used in growth() to estimate diameter increment

}

void Tree::setAge(const int age, const float treeheight)
{
    mAge = age;
    if (age==0) {
        // estimate age using the tree height
        mAge = mSpecies->estimateAge(treeheight);
    }
}

//////////////////////////////////////////////////
////  Light functions (Pattern-stuff)
//////////////////////////////////////////////////

#define NOFULLDBG
//#define NOFULLOPT


void Tree::applyLIP()
{
    if (!mStamp)
        return;
    Q_ASSERT(mGrid!=0 && mStamp!=0 && mRU!=0);
    QPoint pos = mPositionIndex;
    int offset = mStamp->offset();
    pos-=QPoint(offset, offset);

    float local_dom; // height of Z* on the current position
    int x,y;
    float value, z, z_zstar;
    int gr_stamp = mStamp->size();

    if (!mGrid->isIndexValid(pos) || !mGrid->isIndexValid(pos+QPoint(gr_stamp, gr_stamp))) {
        // this should not happen because of the buffer
        return;
    }
    int grid_y = pos.y();
    for (y=0;y<gr_stamp; ++y) {

        float *grid_value_ptr = mGrid->ptr(pos.x(), grid_y);
        int grid_x = pos.x();
        for (x=0;x<gr_stamp;++x, ++grid_x, ++grid_value_ptr) {
            // suppose there is no stamping outside
            value = (*mStamp)(x,y); // stampvalue
            //if (value>0.f) {
                local_dom = (*mHeightGrid)(grid_x/cPxPerHeight, grid_y/cPxPerHeight).height;
                z = std::max(mHeight - (*mStamp).distanceToCenter(x,y), 0.f); // distance to center = height (45 degree line)
                z_zstar = (z>=local_dom)?1.f:z/local_dom;
                value = 1. - value*mOpacity * z_zstar; // calculated value
                value = std::max(value, 0.02f); // limit value

                *grid_value_ptr *= value;
            //}

        }
        grid_y++;
    }

    m_statPrint++; // count # of stamp applications...
}

/// helper function for gluing the edges together
/// index: index at grid
/// count: number of pixels that are the simulation area (e.g. 100m and 2m pixel -> 50)
/// buffer: size of buffer around simulation area (in pixels)
inline int torusIndex(int index, int count, int buffer, int ru_index)
{
    return buffer + ru_index + (index-buffer+count)%count;
}


/** Apply LIPs. This "Torus" functions wraps the influence at the edges of a 1ha simulation area.
  */
void Tree::applyLIP_torus()
{
    if (!mStamp)
        return;
    Q_ASSERT(mGrid!=0 && mStamp!=0 && mRU!=0);
    int bufferOffset = mGrid->indexAt(QPointF(0.,0.)).x(); // offset of buffer
    QPoint pos = QPoint((mPositionIndex.x()-bufferOffset)%cPxPerRU  + bufferOffset,
                        (mPositionIndex.y()-bufferOffset)%cPxPerRU + bufferOffset); // offset within the ha
    QPoint ru_offset = QPoint(mPositionIndex.x() - pos.x(), mPositionIndex.y() - pos.y()); // offset of the corner of the resource index

    int offset = mStamp->offset();
    pos-=QPoint(offset, offset);

    float local_dom; // height of Z* on the current position
    int x,y;
    float value;
    int gr_stamp = mStamp->size();
    int grid_x, grid_y;
    float *grid_value;
    if (!mGrid->isIndexValid(pos) || !mGrid->isIndexValid(pos+QPoint(gr_stamp, gr_stamp))) {
        // todo: in this case we should use another algorithm!!! necessary????
        return;
    }
    float z, z_zstar;
    int xt, yt; // wraparound coordinates
    for (y=0;y<gr_stamp; ++y) {
        grid_y = pos.y() + y;
        yt = torusIndex(grid_y, cPxPerRU,bufferOffset, ru_offset.y()); // 50 cells per 100m
        for (x=0;x<gr_stamp;++x) {
            // suppose there is no stamping outside
            grid_x = pos.x() + x;
            xt = torusIndex(grid_x,cPxPerRU,bufferOffset, ru_offset.x());

            local_dom = mHeightGrid->valueAtIndex(xt/cPxPerHeight,yt/cPxPerHeight).height;

            z = std::max(mHeight - (*mStamp).distanceToCenter(x,y), 0.f); // distance to center = height (45 degree line)
            z_zstar = (z>=local_dom)?1.f:z/local_dom;
            value = (*mStamp)(x,y); // stampvalue
            value = 1. - value*mOpacity * z_zstar; // calculated value
            // old: value = 1. - value*mOpacity / local_dom; // calculated value
            value = qMax(value, 0.02f); // limit value

            grid_value = mGrid->ptr(xt, yt); // use wraparound coordinates
            *grid_value *= value;
        }
    }

    m_statPrint++; // count # of stamp applications...
}

/** heightGrid()
  This function calculates the "dominant height field". This grid is coarser as the fine-scaled light-grid.
*/
void Tree::heightGrid()
{

    QPoint p = QPoint(mPositionIndex.x()/cPxPerHeight, mPositionIndex.y()/cPxPerHeight); // pos of tree on height grid

    // count trees that are on height-grid cells (used for stockable area)
    mHeightGrid->valueAtIndex(p).increaseCount();
    if (mHeight > mHeightGrid->valueAtIndex(p).height)
        mHeightGrid->valueAtIndex(p).height=mHeight;

    int r = mStamp->reader()->offset(); // distance between edge and the center pixel. e.g.: if r = 2 -> stamp=5x5
    int index_eastwest = mPositionIndex.x() % cPxPerHeight; // 4: very west, 0 east edge
    int index_northsouth = mPositionIndex.y() % cPxPerHeight; // 4: northern edge, 0: southern edge
    if (index_eastwest - r < 0) { // east
        mHeightGrid->valueAtIndex(p.x()-1, p.y()).height=qMax(mHeightGrid->valueAtIndex(p.x()-1, p.y()).height,mHeight);
    }
    if (index_eastwest + r >= cPxPerHeight) {  // west
        mHeightGrid->valueAtIndex(p.x()+1, p.y()).height=qMax(mHeightGrid->valueAtIndex(p.x()+1, p.y()).height,mHeight);
    }
    if (index_northsouth - r < 0) {  // south
        mHeightGrid->valueAtIndex(p.x(), p.y()-1).height=qMax(mHeightGrid->valueAtIndex(p.x(), p.y()-1).height,mHeight);
    }
    if (index_northsouth + r >= cPxPerHeight) {  // north
        mHeightGrid->valueAtIndex(p.x(), p.y()+1).height=qMax(mHeightGrid->valueAtIndex(p.x(), p.y()+1).height,mHeight);
    }


    // without spread of the height grid

//    // height of Z*
//    const float cellsize = mHeightGrid->cellsize();
//
//    int index_eastwest = mPositionIndex.x() % cPxPerHeight; // 4: very west, 0 east edge
//    int index_northsouth = mPositionIndex.y() % cPxPerHeight; // 4: northern edge, 0: southern edge
//    int dist[9];
//    dist[3] = index_northsouth * 2 + 1; // south
//    dist[1] = index_eastwest * 2 + 1; // west
//    dist[5] = 10 - dist[3]; // north
//    dist[7] = 10 - dist[1]; // east
//    dist[8] = qMax(dist[5], dist[7]); // north-east
//    dist[6] = qMax(dist[3], dist[7]); // south-east
//    dist[0] = qMax(dist[3], dist[1]); // south-west
//    dist[2] = qMax(dist[5], dist[1]); // north-west
//    dist[4] = 0; // center cell
//    /* the scheme of indices is as follows:  if sign(ix)= -1, if ix<0, 0 for ix=0, 1 for ix>0 (detto iy), then:
//       index = 4 + 3*sign(ix) + sign(iy) transforms combinations of directions to unique ids (0..8), which are used above.
//        e.g.: sign(ix) = -1, sign(iy) = 1 (=north-west) -> index = 4 + -3 + 1 = 2
//    */
//
//
//    int ringcount = int(floor(mHeight / cellsize)) + 1;
//    int ix, iy;
//    int ring;
//    float hdom;
//
//    for (ix=-ringcount;ix<=ringcount;ix++)
//        for (iy=-ringcount; iy<=+ringcount; iy++) {
//        ring = qMax(abs(ix), abs(iy));
//        QPoint pos(ix+p.x(), iy+p.y());
//        if (mHeightGrid->isIndexValid(pos)) {
//            float &rHGrid = mHeightGrid->valueAtIndex(pos).height;
//            if (rHGrid > mHeight) // skip calculation if grid is higher than tree
//                continue;
//            int direction = 4 + (ix?(ix<0?-3:3):0) + (iy?(iy<0?-1:1):0); // 4 + 3*sgn(x) + sgn(y)
//            hdom = mHeight - dist[direction];
//            if (ring>1)
//                hdom -= (ring-1)*10;
//
//            rHGrid = qMax(rHGrid, hdom); // write value
//        } // is valid
//    } // for (y)
}

void Tree::heightGrid_torus()
{
    // height of Z*

    QPoint p = QPoint(mPositionIndex.x()/cPxPerHeight, mPositionIndex.y()/cPxPerHeight); // pos of tree on height grid
    int bufferOffset = mHeightGrid->indexAt(QPointF(0.,0.)).x(); // offset of buffer (i.e.: size of buffer in height-pixels)
    p.setX((p.x()-bufferOffset)%10 + bufferOffset); // 10: 10 x 10m pixeln in 100m
    p.setY((p.y()-bufferOffset)%10 + bufferOffset);


    // torus coordinates: ru_offset = coords of lower left corner of 1ha patch
    QPoint ru_offset =QPoint(mPositionIndex.x()/cPxPerHeight - p.x(), mPositionIndex.y()/cPxPerHeight - p.y());

    // count trees that are on height-grid cells (used for stockable area)
    HeightGridValue &v = mHeightGrid->valueAtIndex(torusIndex(p.x(),10,bufferOffset,ru_offset.x()),
                                                   torusIndex(p.y(),10,bufferOffset,ru_offset.y()));
    v.increaseCount();
    v.height = qMax(v.height, mHeight);


    int r = mStamp->reader()->offset(); // distance between edge and the center pixel. e.g.: if r = 2 -> stamp=5x5
    int index_eastwest = mPositionIndex.x() % cPxPerHeight; // 4: very west, 0 east edge
    int index_northsouth = mPositionIndex.y() % cPxPerHeight; // 4: northern edge, 0: southern edge
    if (index_eastwest - r < 0) { // east
        HeightGridValue &v = mHeightGrid->valueAtIndex(torusIndex(p.x()-1,10,bufferOffset,ru_offset.x()),
                                                       torusIndex(p.y(),10,bufferOffset,ru_offset.y()));
        v.height = qMax(v.height, mHeight);
    }
    if (index_eastwest + r >= cPxPerHeight) {  // west
        HeightGridValue &v = mHeightGrid->valueAtIndex(torusIndex(p.x()+1,10,bufferOffset,ru_offset.x()),
                                                       torusIndex(p.y(),10,bufferOffset,ru_offset.y()));
        v.height = qMax(v.height, mHeight);
    }
    if (index_northsouth - r < 0) {  // south
        HeightGridValue &v = mHeightGrid->valueAtIndex(torusIndex(p.x(),10,bufferOffset,ru_offset.x()),
                                                       torusIndex(p.y()-1,10,bufferOffset,ru_offset.y()));
        v.height = qMax(v.height, mHeight);
    }
    if (index_northsouth + r >= cPxPerHeight) {  // north
        HeightGridValue &v = mHeightGrid->valueAtIndex(torusIndex(p.x(),10,bufferOffset,ru_offset.x()),
                                                       torusIndex(p.y()+1,10,bufferOffset,ru_offset.y()));
        v.height = qMax(v.height, mHeight);
    }




//    int index_eastwest = mPositionIndex.x() % cPxPerHeight; // 4: very west, 0 east edge
//    int index_northsouth = mPositionIndex.y() % cPxPerHeight; // 4: northern edge, 0: southern edge
//    int dist[9];
//    dist[3] = index_northsouth * 2 + 1; // south
//    dist[1] = index_eastwest * 2 + 1; // west
//    dist[5] = 10 - dist[3]; // north
//    dist[7] = 10 - dist[1]; // east
//    dist[8] = qMax(dist[5], dist[7]); // north-east
//    dist[6] = qMax(dist[3], dist[7]); // south-east
//    dist[0] = qMax(dist[3], dist[1]); // south-west
//    dist[2] = qMax(dist[5], dist[1]); // north-west
//    dist[4] = 0; // center cell
//    /* the scheme of indices is as follows:  if sign(ix)= -1, if ix<0, 0 for ix=0, 1 for ix>0 (detto iy), then:
//       index = 4 + 3*sign(ix) + sign(iy) transforms combinations of directions to unique ids (0..8), which are used above.
//        e.g.: sign(ix) = -1, sign(iy) = 1 (=north-west) -> index = 4 + -3 + 1 = 2
//    */
//
//
//    int ringcount = int(floor(mHeight / cellsize)) + 1;
//    int ix, iy;
//    int ring;
//    float hdom;
//    for (ix=-ringcount;ix<=ringcount;ix++)
//        for (iy=-ringcount; iy<=+ringcount; iy++) {
//        ring = qMax(abs(ix), abs(iy));
//        QPoint pos(ix+p.x(), iy+p.y());
//        QPoint p_torus(torusIndex(pos.x(),10,bufferOffset,ru_offset.x()),
//                       torusIndex(pos.y(),10,bufferOffset,ru_offset.y()));
//        if (mHeightGrid->isIndexValid(p_torus)) {
//            float &rHGrid = mHeightGrid->valueAtIndex(p_torus.x(),p_torus.y()).height;
//            if (rHGrid > mHeight) // skip calculation if grid is higher than tree
//                continue;
//            int direction = 4 + (ix?(ix<0?-3:3):0) + (iy?(iy<0?-1:1):0); // 4 + 3*sgn(x) + sgn(y)
//            hdom = mHeight - dist[direction];
//            if (ring>1)
//                hdom -= (ring-1)*10;
//
//            rHGrid = qMax(rHGrid, hdom); // write value
//        } // is valid
//    } // for (y)
}


/** reads the light influence field value for a tree.
    The LIF field is scanned within the crown area of the focal tree, and the influence of
    the focal tree is "subtracted" from the LIF values.
    Finally, the "LRI correction" is applied.
    see http://iland.boku.ac.at/competition+for+light for details.
  */
void Tree::readLIF()
{
    if (!mStamp)
        return;
    const Stamp *reader = mStamp->reader();
    if (!reader)
        return;
    QPoint pos_reader = mPositionIndex;
    const float outside_area_factor = 0.1f; //

    int offset_reader = reader->offset();
    int offset_writer = mStamp->offset();
    int d_offset = offset_writer - offset_reader; // offset on the *stamp* to the crown-cells

    pos_reader-=QPoint(offset_reader, offset_reader);

    float local_dom;

    int x,y;
    double sum=0.;
    double value, own_value;
    float *grid_value;
    float z, z_zstar;
    int reader_size = reader->size();
    int rx = pos_reader.x();
    int ry = pos_reader.y();
    for (y=0;y<reader_size; ++y, ++ry) {
        grid_value = mGrid->ptr(rx, ry);
        for (x=0;x<reader_size;++x) {

            const HeightGridValue &hgv = mHeightGrid->constValueAtIndex((rx+x)/cPxPerHeight, ry/cPxPerHeight); // the height grid value, ry: gets ++ed in outer loop, rx not
            local_dom = hgv.height;
            z = std::max(mHeight - reader->distanceToCenter(x,y), 0.f); // distance to center = height (45 degree line)
            z_zstar = (z>=local_dom)?1.f:z/local_dom;

            own_value = 1. - mStamp->offsetValue(x,y,d_offset)*mOpacity * z_zstar;
            own_value = qMax(own_value, 0.02);
            value =  *grid_value++ / own_value; // remove impact of focal tree
            // additional punishment if pixel is outside:
            if (hgv.isForestOutside())
               value *= outside_area_factor;
            
            //qDebug() << x << y << local_dom << z << z_zstar << own_value << value << *(grid_value-1) << (*reader)(x,y) << mStamp->offsetValue(x,y,d_offset);
            //if (value>0.)
            sum += value * (*reader)(x,y);

        }
    }
    mLRI = sum;
    // LRI correction...
    double hrel = mHeight / mHeightGrid->valueAtIndex(mPositionIndex.x()/cPxPerHeight, mPositionIndex.y()/cPxPerHeight).height;
    if (hrel<1.)
        mLRI = species()->speciesSet()->LRIcorrection(mLRI, hrel);


    if (mLRI > 1.)
        mLRI = 1.;

    // Finally, add LRI of this Tree to the ResourceUnit!
    mRU->addWLA(mLeafArea, mLRI);

    //qDebug() << "Tree #"<< id() << "value" << sum << "Impact" << mImpact;
}

/// Torus version of read stamp (glued edges)
void Tree::readLIF_torus()
{
    if (!mStamp)
        return;
    const Stamp *reader = mStamp->reader();
    if (!reader)
        return;
    int bufferOffset = mGrid->indexAt(QPointF(0.,0.)).x(); // offset of buffer

    QPoint pos_reader = QPoint((mPositionIndex.x()-bufferOffset)%cPxPerRU + bufferOffset,
                               (mPositionIndex.y()-bufferOffset)%cPxPerRU + bufferOffset); // offset within the ha
    QPoint ru_offset = QPoint(mPositionIndex.x() - pos_reader.x(), mPositionIndex.y() - pos_reader.y()); // offset of the corner of the resource index

    int offset_reader = reader->offset();
    int offset_writer = mStamp->offset();
    int d_offset = offset_writer - offset_reader; // offset on the *stamp* to the crown-cells

    pos_reader-=QPoint(offset_reader, offset_reader);

    float local_dom;

    int x,y;
    double sum=0.;
    double value, own_value;
    float *grid_value;
    float z, z_zstar;
    int reader_size = reader->size();
    int rx = pos_reader.x();
    int ry = pos_reader.y();
    int xt, yt; // wrapped coords

    for (y=0;y<reader_size; ++y) {
        yt = torusIndex(ry+y,cPxPerRU, bufferOffset, ru_offset.y());
        for (x=0;x<reader_size;++x) {
            xt = torusIndex(rx+x,cPxPerRU, bufferOffset, ru_offset.x());
            grid_value = mGrid->ptr(xt,yt);

            local_dom = mHeightGrid->valueAtIndex(xt/cPxPerHeight, yt/cPxPerHeight).height; // ry: gets ++ed in outer loop, rx not
            z = std::max(mHeight - reader->distanceToCenter(x,y), 0.f); // distance to center = height (45 degree line)
            z_zstar = (z>=local_dom)?1.f:z/local_dom;

            own_value = 1. - mStamp->offsetValue(x,y,d_offset)*mOpacity * z_zstar;
            // old: own_value = 1. - mStamp->offsetValue(x,y,d_offset)*mOpacity / local_dom; // old: dom_height;
            own_value = qMax(own_value, 0.02);
            value =  *grid_value++ / own_value; // remove impact of focal tree

            // debug for one tree in HJA
            //if (id()==178020)
            //    qDebug() << x << y << xt << yt << *grid_value << local_dom << own_value << value << (*reader)(x,y);
            //if (_isnan(value))
            //    qDebug() << "isnan" << id();
            if (value * (*reader)(x,y)>1.)
                qDebug() << "LIFTorus: value>1.";
            sum += value * (*reader)(x,y);

            //} // isIndexValid
        }
    }
    mLRI = sum;

    // LRI correction...
    double hrel = mHeight / mHeightGrid->valueAtIndex(mPositionIndex.x()/cPxPerHeight, mPositionIndex.y()/cPxPerHeight).height;
    if (hrel<1.)
        mLRI = species()->speciesSet()->LRIcorrection(mLRI, hrel);


    if (isnan(mLRI)) {
        qDebug() << "LRI invalid (nan)!" << id();
        mLRI=0.;
        //qDebug() << reader->dump();
    }
    if (mLRI > 1.)
        mLRI = 1.;
    //qDebug() << "Tree #"<< id() << "value" << sum << "Impact" << mImpact;

    // Finally, add LRI of this Tree to the ResourceUnit!
    mRU->addWLA(mLeafArea, mLRI);
}


void Tree::resetStatistics()
{
    m_statPrint=0;
    m_statCreated=0;
    m_statAboveZ=0;
    m_nextId=1;
}

void Tree::calcLightResponse()
{
    // calculate a light response from lri:
    // http://iland.boku.ac.at/individual+tree+light+availability
    double lri = limit(mLRI * mRU->LRImodifier(), 0., 1.); // Eq. (3)
    mLightResponse = mSpecies->lightResponse(lri); // Eq. (4)
    mRU->addLR(mLeafArea, mLightResponse);

}

//////////////////////////////////////////////////
////  Growth Functions
//////////////////////////////////////////////////

/** grow() is the main function of the yearly tree growth.
  The main steps are:
  - Production of GPP/NPP   @sa http://iland.boku.ac.at/primary+production http://iland.boku.ac.at/individual+tree+light+availability
  - Partitioning of NPP to biomass compartments of the tree @sa http://iland.boku.ac.at/allocation
  - Growth of the stem http://iland.boku.ac.at/stem+growth (???)
  Further activties: * the age of the tree is increased
                     * the mortality sub routine is executed
                     * seeds are produced */
void Tree::grow()
{
    TreeGrowthData d;
    mAge++; // increase age
    // step 1: get "interception area" of the tree individual [m2]
    // the sum of all area of all trees of a unit equal the total stocked area * interception_factor(Beer-Lambert)
    double effective_area = mRU->interceptedArea(mLeafArea, mLightResponse);

    // step 2: calculate GPP of the tree based
    // (1) get the amount of GPP for a "unit area" of the tree species
    double raw_gpp_per_area = mRU->resourceUnitSpecies(species()).prod3PG().GPPperArea();
    // (2) GPP (without aging-effect) in kg Biomass / year
    double raw_gpp = raw_gpp_per_area * effective_area;

    // apply aging according to the state of the individuum
    const double aging_factor = mSpecies->aging(mHeight, mAge);
    mRU->addTreeAging(mLeafArea, aging_factor);
    double gpp = raw_gpp * aging_factor; //
    d.NPP = gpp * cAutotrophicRespiration; // respiration loss (0.47), cf. Waring et al 1998.

    //DBGMODE(
        if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreeNPP) && isDebugging()) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreeNPP);
            dumpList(out); // add tree headers
            out << mLRI * mRU->LRImodifier() << mLightResponse << effective_area << raw_gpp << gpp << d.NPP << aging_factor;
        }
    //); // DBGMODE()
    if (d.NPP>0.)
        partitioning(d); // split npp to compartments and grow (diameter, height)

    // mortality
    if (Model::settings().mortalityEnabled)
        mortality(d);

    mStressIndex = d.stress_index;

    if (!isDead())
        mRU->resourceUnitSpecies(species()).statistics().add(this, &d);

    // regeneration
    mSpecies->seedProduction(mAge, mHeight, mPositionIndex);

}

/** partitioning of this years assimilates (NPP) to biomass compartments.
  Conceptionally, the algorithm is based on Duursma, 2007.
  @sa http://iland.boku.ac.at/allocation */
inline void Tree::partitioning(TreeGrowthData &d)
{
    double npp = d.NPP;
    // add content of reserve pool
    npp += mNPPReserve;
    const double foliage_mass_allo = species()->biomassFoliage(mDbh);
    const double reserve_size = foliage_mass_allo * (1. + mSpecies->finerootFoliageRatio());
    double refill_reserve = qMin(reserve_size, (1. + mSpecies->finerootFoliageRatio())*mFoliageMass); // not always try to refill reserve 100%

    double apct_wood, apct_root, apct_foliage; // allocation percentages (sum=1) (eta)
    ResourceUnitSpecies &rus = mRU->resourceUnitSpecies(species());
    // turnover rates
    const double &to_fol = species()->turnoverLeaf();
    const double &to_root = species()->turnoverRoot();
    // the turnover rate of wood depends on the size of the reserve pool:


    double to_wood = refill_reserve / (mWoodyMass + refill_reserve);

    apct_root = rus.prod3PG().rootFraction();
    d.NPP_above = d.NPP * (1. - apct_root); // aboveground: total NPP - fraction to roots
    double b_wf = species()->allometricRatio_wf(); // ratio of allometric exponents (b_woody / b_foliage)

    // Duursma 2007, Eq. (20)
    apct_wood = (foliage_mass_allo*to_wood/npp + b_wf*(1.-apct_root) - b_wf*foliage_mass_allo*to_fol/npp) / ( foliage_mass_allo/mWoodyMass + b_wf );

    apct_wood = limit(apct_wood, 0., 1.-apct_root);

    apct_foliage = 1. - apct_root - apct_wood;


    DBGMODE(
            if (apct_foliage<0 || apct_wood<0)
                qDebug() << "transfer to foliage or wood < 0";
             if (npp<0)
                 qDebug() << "NPP < 0";
            );

    // Change of biomass compartments
    double sen_root = mFineRootMass * to_root;
    double sen_foliage = mFoliageMass * to_fol;
    if (ru()->snag())
        ru()->snag()->addTurnoverLitter(this->species(), sen_foliage, sen_root);

    // Roots
    // http://iland.boku.ac.at/allocation#belowground_NPP
    mFineRootMass -= sen_root; // reduce only fine root pool
    double delta_root = apct_root * npp;
    // 1st, refill the fine root pool
    double fineroot_miss = mFoliageMass * mSpecies->finerootFoliageRatio() - mFineRootMass;
    if (fineroot_miss>0.){
        double delta_fineroot = qMin(fineroot_miss, delta_root);
        mFineRootMass += delta_fineroot;
        delta_root -= delta_fineroot;
    }
    // 2nd, the rest of NPP allocated to roots go to coarse roots
    double max_coarse_root = species()->biomassRoot(mDbh);
    mCoarseRootMass += delta_root;
    if (mCoarseRootMass > max_coarse_root) {
        // if the coarse root pool exceeds the value given by the allometry, then the
        // surplus is accounted as turnover
        if (ru()->snag())
            ru()->snag()->addTurnoverWood(species(), mCoarseRootMass-max_coarse_root);

        mCoarseRootMass = max_coarse_root;
    }

    // Foliage
    double delta_foliage = apct_foliage * npp - sen_foliage;
    mFoliageMass += delta_foliage;
    if (isnan(mFoliageMass))
        qDebug() << "foliage mass invalid!";
    if (mFoliageMass<0.) mFoliageMass=0.; // limit to zero

    mLeafArea = mFoliageMass * species()->specificLeafArea(); // update leaf area

    // stress index: different varaints at denominator: to_fol*foliage_mass = leafmass to rebuild,
    // foliage_mass_allo: simply higher chance for stress
    // note: npp = NPP + reserve (see above)
    d.stress_index =qMax(1. - (npp) / ( to_fol*foliage_mass_allo + to_root*foliage_mass_allo*species()->finerootFoliageRatio() + reserve_size), 0.);

    // Woody compartments
    // see also: http://iland.boku.ac.at/allocation#reserve_and_allocation_to_stem_growth
    // (1) transfer to reserve pool
    double gross_woody = apct_wood * npp;
    double to_reserve = qMin(reserve_size, gross_woody);
    mNPPReserve = to_reserve;
    double net_woody = gross_woody - to_reserve;
    double net_stem = 0.;
    mDbhDelta = 0.;


    if (net_woody > 0.) {
        // (2) calculate part of increment that is dedicated to the stem (which is a function of diameter)
        net_stem = net_woody * species()->allometricFractionStem(mDbh);
        d.NPP_stem = net_stem;
        mWoodyMass += net_woody;
        //  (3) growth of diameter and height baseed on net stem increment
        grow_diameter(d);
    }

    //DBGMODE(
     if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreePartition)
         && isDebugging() ) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreePartition);
            dumpList(out); // add tree headers
            out << npp << apct_foliage << apct_wood << apct_root
                    << delta_foliage << net_woody << delta_root << mNPPReserve << net_stem << d.stress_index;
     }

    //); // DBGMODE()
    DBGMODE(
      if (mWoodyMass<0. || mWoodyMass>50000 || mFoliageMass<0. || mFoliageMass>2000. || mCoarseRootMass<0. || mCoarseRootMass>30000
         || mNPPReserve>4000.) {
         qDebug() << "Tree:partitioning: invalid or unlikely pools.";
         qDebug() << GlobalSettings::instance()->debugListCaptions(GlobalSettings::DebugOutputs(0));
         DebugList dbg; dumpList(dbg);
         qDebug() << dbg;
     } );

    /*DBG_IF_X(mId == 1 , "Tree::partitioning", "dump", dump()
             + QString("npp %1 npp_reserve %9 sen_fol %2 sen_stem %3 sen_root %4 net_fol %5 net_stem %6 net_root %7 to_reserve %8")
               .arg(npp).arg(senescence_foliage).arg(senescence_stem).arg(senescence_root)
               .arg(net_foliage).arg(net_stem).arg(net_root).arg(to_reserve).arg(mNPPReserve) );*/

}


/** Determination of diamter and height growth based on increment of the stem mass (@p net_stem_npp).
    Refer to XXX for equations and variables.
    This function updates the dbh and height of the tree.
    The equations are based on dbh in meters! */
inline void Tree::grow_diameter(TreeGrowthData &d)
{
    // determine dh-ratio of increment
    // height increment is a function of light competition:
    double hd_growth = relative_height_growth(); // hd of height growth
    double d_m = mDbh / 100.; // current diameter in [m]
    double net_stem_npp = d.NPP_stem;

    const double d_delta_m = mDbhDelta / 100.; // increment of last year in [m]

    const double mass_factor = species()->volumeFactor() * species()->density();
    double stem_mass = mass_factor * d_m*d_m * mHeight; // result: kg, dbh[cm], h[meter]

    // factor is in diameter increment per NPP [m/kg]
    double factor_diameter = 1. / (  mass_factor * (d_m + d_delta_m)*(d_m + d_delta_m) * ( 2. * mHeight/d_m + hd_growth) );
    double delta_d_estimate = factor_diameter * net_stem_npp; // estimated dbh-inc using last years increment

    // using that dbh-increment we estimate a stem-mass-increment and the residual (Eq. 9)
    double stem_estimate = mass_factor * (d_m + delta_d_estimate)*(d_m + delta_d_estimate)*(mHeight + delta_d_estimate*hd_growth);
    double stem_residual = stem_estimate - (stem_mass + net_stem_npp);

    // the final increment is then:
    double d_increment = factor_diameter * (net_stem_npp - stem_residual); // Eq. (11)
    double res_final  = 0.;
    if (fabs(stem_residual) > 1.) {

        // calculate final residual in stem
        res_final = mass_factor * (d_m + d_increment)*(d_m + d_increment)*(mHeight + d_increment*hd_growth)-((stem_mass + net_stem_npp));
        if (fabs(res_final)>1.) {
            // for large errors in stem biomass due to errors in diameter increment (> 1kg), we solve the increment iteratively.
            // first, increase increment with constant step until we overestimate the first time
            // then,
            d_increment = 0.02; // start with 2cm increment
            bool reached_error = false;
            double step=0.01; // step-width 1cm
            double est_stem;
            do {
                est_stem = mass_factor * (d_m + d_increment)*(d_m + d_increment)*(mHeight + d_increment*hd_growth); // estimate with current increment
                stem_residual = est_stem - (stem_mass + net_stem_npp);

                if (fabs(stem_residual) <1.) // finished, if stem residual below 1kg
                    break;
                if (stem_residual > 0.) {
                    d_increment -= step;
                    reached_error=true;
                } else {
                    d_increment += step;
                }
                if (reached_error)
                    step /= 2.;
            } while (step>0.00001); // continue until diameter "accuracy" falls below 1/100mm
        }
    }

    if (d_increment<0.f)
        qDebug() << "Tree::grow_diameter: d_inc < 0.";
    DBG_IF_X(d_increment<0. || d_increment>0.1, "Tree::grow_dimater", "increment out of range.", dump()
             + QString("\nhdz %1 factor_diameter %2 stem_residual %3 delta_d_estimate %4 d_increment %5 final residual(kg) %6")
               .arg(hd_growth).arg(factor_diameter).arg(stem_residual).arg(delta_d_estimate).arg(d_increment)
               .arg( mass_factor * (mDbh + d_increment)*(mDbh + d_increment)*(mHeight + d_increment*hd_growth)-((stem_mass + net_stem_npp)) ));

    //DBGMODE(
        // do not calculate res_final twice if already done
        DBG_IF_X( (res_final==0.?fabs(mass_factor * (d_m + d_increment)*(d_m + d_increment)*(mHeight + d_increment*hd_growth)-((stem_mass + net_stem_npp))):res_final) > 1, "Tree::grow_diameter", "final residual stem estimate > 1kg", dump());
        DBG_IF_X(d_increment > 10. || d_increment*hd_growth >10., "Tree::grow_diameter", "growth out of bound:",QString("d-increment %1 h-increment %2 ").arg(d_increment).arg(d_increment*hd_growth/100.) + dump());

        if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreeGrowth) && isDebugging() ) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreeGrowth);
            dumpList(out); // add tree headers
            out << net_stem_npp << stem_mass << hd_growth << factor_diameter << delta_d_estimate*100 << d_increment*100;
        }

    //); // DBGMODE()

    d_increment = qMax(d_increment, 0.);

    // update state variables
    mDbh += d_increment*100; // convert from [m] to [cm]
    mDbhDelta = d_increment*100; // save for next year's growth
    mHeight += d_increment * hd_growth;

    // update state of LIP stamp and opacity
    mStamp = species()->stamp(mDbh, mHeight); // get new stamp for updated dimensions
    // calculate the CrownFactor which reflects the opacity of the crown
    const double k=Model::settings().lightExtinctionCoefficientOpacity;
    mOpacity = 1. - exp(-k * mLeafArea / mStamp->crownArea());

}


/// return the HD ratio of this year's increment based on the light status.
inline double Tree::relative_height_growth()
{
    double hd_low, hd_high;
    mSpecies->hdRange(mDbh, hd_low, hd_high);

    DBG_IF_X(hd_low>hd_high, "Tree::relative_height_growth", "hd low higher dann hd_high for ", dump());
    DBG_IF_X(hd_low < 10 || hd_high>250, "Tree::relative_height_growth", "hd out of range ", dump() + QString(" hd-low: %1 hd-high: %2").arg(hd_low).arg(hd_high));

    // scale according to LRI: if receiving much light (LRI=1), the result is hd_low (for open grown trees)
    // use the corrected LRI (see tracker#11)
    double lri = limit(mLRI * mRU->LRImodifier(),0.,1.);
    double hd_ratio = hd_high - (hd_high-hd_low)*lri;
    return hd_ratio;
}

/** This function is called if a tree dies.
  @sa ResourceUnit::cleanTreeList(), remove() */
void Tree::die(TreeGrowthData *d)
{
    setFlag(Tree::TreeDead, true); // set flag that tree is dead
    mRU->treeDied();
    ResourceUnitSpecies &rus = mRU->resourceUnitSpecies(species());
    rus.statisticsDead().add(this, d); // add tree to statistics
    recordRemovedVolume(TreeDeath);
    if (ru()->snag())
        ru()->snag()->addMortality(this);
}

/// remove a tree (most likely due to harvest) from the system.
void Tree::remove(double removeFoliage, double removeBranch, double removeStem )
{
    setFlag(Tree::TreeDead, true); // set flag that tree is dead
    mRU->treeDied();
    ResourceUnitSpecies &rus = mRU->resourceUnitSpecies(species());
    rus.statisticsMgmt().add(this, 0);
    recordRemovedVolume(TreeHarvest);

    if (ru()->snag())
        ru()->snag()->addHarvest(this, removeStem, removeBranch, removeFoliage);
}

/// remove the tree due to an special event (disturbance)
/// this is +- the same as die().
void Tree::removeDisturbance(const double stem_to_soil_fraction, const double stem_to_snag_fraction, const double branch_to_soil_fraction, const double branch_to_snag_fraction, const double foliage_to_soil_fraction)
{
    setFlag(Tree::TreeDead, true); // set flag that tree is dead
    mRU->treeDied();
    ResourceUnitSpecies &rus = mRU->resourceUnitSpecies(species());
    rus.statisticsDead().add(this, 0);
    recordRemovedVolume(TreeDisturbance);

    if (ru()->snag())
        ru()->snag()->addDisturbance(this, stem_to_snag_fraction, stem_to_soil_fraction, branch_to_snag_fraction, branch_to_soil_fraction, foliage_to_soil_fraction);
}

/// remove a part of the biomass of the tree, e.g. due to fire.
void Tree::removeBiomassOfTree(const double removeFoliageFraction, const double removeBranchFraction, const double removeStemFraction)
{
    mFoliageMass *= 1. - removeFoliageFraction;
    mWoodyMass *= (1. - removeStemFraction);
    // we have a problem with the branches: this currently cannot be done properly!
    (void) removeBranchFraction; // silence warning
}

void Tree::setHeight(const float height)
{
    if (height<=0. || height>150.)
        qWarning() << "trying to set tree height to invalid value:" << height << " for tree on RU:" << (mRU?mRU->boundingBox():QRect());
    mHeight=height;
}

void Tree::mortality(TreeGrowthData &d)
{
    // death if leaf area is 0
    if (mFoliageMass<0.00001)
        die();

    double p_death,  p_stress, p_intrinsic;
    p_intrinsic = species()->deathProb_intrinsic();
    p_stress = species()->deathProb_stress(d.stress_index);
    p_death =  p_intrinsic + p_stress;
    double p = drandom(); //0..1
    if (p<p_death) {
        // die...
        die();
    }
}

void Tree::recordRemovedVolume(TreeRemovalType reason)
{
    // add the volume of the current tree to the height grid
    // this information is used to track the removed volume for stands based on grids.
    ABE::ForestManagementEngine *abe = GlobalSettings::instance()->model()->ABEngine();
    if (abe)
        abe->addTreeRemoval(this, (int)reason);
}

//////////////////////////////////////////////////
////  value functions
//////////////////////////////////////////////////

double Tree::volume() const
{
    /// @see Species::volumeFactor() for details
    const double volume_factor = species()->volumeFactor();
    const double volume =  volume_factor * mDbh*mDbh*mHeight * 0.0001; // dbh in cm: cm/100 * cm/100 = cm*cm * 0.0001 = m2
    return volume;
}

/// return the basal area in m2
double Tree::basalArea() const
{
    // A = r^2 * pi = d/2*pi; from cm->m: d/200
    const double b = (mDbh/200.)*(mDbh/200.)*M_PI;
    return b;
}

