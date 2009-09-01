
#include "tree.h"
#include "grid.h"

#include "stamp.h"
#include "species.h"
#include "ressourceunit.h"

// static varaibles
FloatGrid *Tree::mGrid = 0;
FloatGrid *Tree::mHeightGrid = 0;
int Tree::m_statPrint=0;
int Tree::m_statAboveZ=0;
int Tree::m_statCreated=0;
int Tree::m_nextId=0;
float Tree::lafactor = 1.;
int Tree::mDebugid = -1;

// lifecycle
Tree::Tree()
{
    mDbh = 0;
    mHeight = 0;
    mSpecies = 0;
    mRU = 0;
    mId = m_nextId++;
    m_statCreated++;
}

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


void Tree::setup()
{
    if (mDbh<=0 || mHeight<=0)
        return;
    // check stamp
   Q_ASSERT_X(mSpecies!=0, "Tree::setup()", "species is NULL");
   mStamp = mSpecies->stamp(mDbh, mHeight);

   calcBiomassCompartments();
   mNPPReserve = mLeafMass; // initial value

}

//////////////////////////////////////////////////
////  Light functions (Pattern-stuff)
//////////////////////////////////////////////////

#define NOFULLDBG
//#define NOFULLOPT
/*
void Tree::applyStamp()
{
    Q_ASSERT(m_grid!=0);
    if (!m_stamp)
        return;

    QPoint pos = m_grid->indexAt(m_Position);
    int offset = m_stamp->offset();
    pos-=QPoint(offset, offset);
    QPoint p;

    float local_dom; // height of Z* on the current position
    int x,y;
    float value;
    QPoint dbg(10,20);
    #ifndef NOFULLDBG
    qDebug() <<"indexstampx indexstamy gridx gridy local_dom_height stampvalue 1-value*la/dom grid_before gridvalue_after";
    #endif
    for (x=0;x<m_stamp->size();++x) {
        for (y=0;y<m_stamp->size(); ++y) {
           p = pos + QPoint(x,y);
           // debug pixel
           //if (p==dbg)
           //    qDebug() << "#" << id() << "value;"<<(*m_stamp)(x,y)<<"domH"<<dom;

           if (m_grid->isIndexValid(p)) {
               // mulitplicative:
               //m_grid->valueAtIndex(p)*=(*m_stamp)(x,y);
               // additiv:
               // multiplicatie, v2
               // a optimization for 2m vs 10m grids: local_dom = m_dominanceGrid->valueAtIndex(p.x()/5, p.y()/5);
               // effect is about 20% of the application time
               local_dom = m_dominanceGrid->valueAt( m_grid->cellCoordinates(p) ); // todo: could be done more effectively (here 2 transormations are performed)...
               if (local_dom<=0.f) {
                   //qDebug() << "invalid height at " << m_grid->cellCoordinates(p) << "of" << local_dom;
                   local_dom = 2.;
               }
               value = (*m_stamp)(x,y);
               value = 1. - value*lafactor / local_dom;
               value = qMax(value, 0.02f);
#ifndef NOFULLDBG
                qDebug() << x << y << p.x() << p.y() << local_dom << (*m_stamp)(x,y) << 1. - (*m_stamp)(x,y)*lafactor / local_dom  << m_grid->valueAtIndex(p) << m_grid->valueAtIndex(p)*value;
#endif

               m_grid->valueAtIndex(p)*= value;
           }
        }
    }

    m_statPrint++; // count # of stamp applications...
}
*/

void Tree::applyStamp()
{
    Q_ASSERT(mGrid!=0 && mStamp!=0 && mRU!=0);

    QPoint pos = mGrid->indexAt(mPosition);
    int offset = mStamp->offset();
    pos-=QPoint(offset, offset);

    float local_dom; // height of Z* on the current position
    int x,y;
    float value;
    int gr_stamp = mStamp->size();
    int grid_x, grid_y;
    float *grid_value;
    if (!mGrid->isIndexValid(pos) || !mGrid->isIndexValid(pos+QPoint(gr_stamp, gr_stamp))) {
        // todo: in this case we should use another algorithm!!!
        return;
    }

    for (y=0;y<gr_stamp; ++y) {
        grid_y = pos.y() + y;
        grid_value = mGrid->ptr(pos.x(), grid_y);
        for (x=0;x<gr_stamp;++x) {
            // suppose there is no stamping outside
            grid_x = pos.x() + x;

            local_dom = mHeightGrid->valueAtIndex(grid_x/5, grid_y/5);
            value = (*mStamp)(x,y); // stampvalue
            value = 1. - value*lafactor / local_dom; // calculated value
            value = qMax(value, 0.02f); // limit value

            *grid_value++ *= value;
        }
    }

    m_statPrint++; // count # of stamp applications...
}

 /*
void Tree::heightGrid_old()
{
    // height of Z*
    const float cellsize = m_dominanceGrid->cellsize();
    if (m_Height < cellsize/2.) {
        float &dom = m_dominanceGrid->valueAt(m_Position); // modifyable reference
        dom = qMax(dom, m_Height/2.f);
    } else {
        QPoint p = m_dominanceGrid->indexAt(m_Position); // pos of tree on height grid

        int ringcount = int(floor(m_Height / cellsize));
        int ix, iy;
        int ring;
        QPoint pos;
        float hdom;
        float h_out = fmod(m_Height, cellsize) / 2.;
        for (ix=-ringcount;ix<=ringcount;ix++)
            for (iy=-ringcount; iy<=+ringcount; iy++) {
            ring = qMax(abs(ix), abs(iy));
            QPoint pos(ix+p.x(), iy+p.y());
            if (m_dominanceGrid->isIndexValid(pos)) {
                // apply value....
                if (ring==0) {
                    hdom = m_Height- cellsize/4.;
                } else if (ring==abs(ringcount)) {
                    // outermost ring: use again height/2.
                    hdom = h_out;
                } else {
                    hdom = m_Height- ring*cellsize;
                }
                m_dominanceGrid->valueAtIndex(pos) = qMax(m_dominanceGrid->valueAtIndex(pos), hdom);
            }

        }
    }

} */

/** heightGrid()
  This function calculates the "dominant height field". This grid is coarser as the fine-scaled light-grid.

*/
void Tree::heightGrid()
{
    // height of Z*
    const float cellsize = mHeightGrid->cellsize();

    QPoint p = mHeightGrid->indexAt(mPosition); // pos of tree on height grid
    QPoint competition_grid = mGrid->indexAt(mPosition);

    int index_eastwest = competition_grid.x() % 5; // 4: very west, 0 east edge
    int index_northsouth = competition_grid.y() % 5; // 4: northern edge, 0: southern edge
    int dist[9];
    dist[3] = index_northsouth * 2 + 1; // south
    dist[1] = index_eastwest * 2 + 1; // west
    dist[5] = 10 - dist[3]; // north
    dist[7] = 10 - dist[1]; // east
    dist[8] = qMax(dist[5], dist[7]); // north-east
    dist[6] = qMax(dist[3], dist[7]); // south-east
    dist[0] = qMax(dist[3], dist[1]); // south-west
    dist[2] = qMax(dist[5], dist[1]); // north-west
    dist[4] = 0; // center cell
    /* the scheme of indices is as follows:  if sign(ix)= -1, if ix<0, 0 for ix=0, 1 for ix>0 (detto iy), then:
       index = 4 + 3*sign(ix) + sign(iy) transforms combinations of directions to unique ids (0..8), which are used above.
        e.g.: sign(ix) = -1, sign(iy) = 1 (=north-west) -> index = 4 + -3 + 1 = 2
    */


    int ringcount = int(floor(mHeight / cellsize)) + 1;
    int ix, iy;
    int ring;
    QPoint pos;
    float hdom;

    for (ix=-ringcount;ix<=ringcount;ix++)
        for (iy=-ringcount; iy<=+ringcount; iy++) {
        ring = qMax(abs(ix), abs(iy));
        QPoint pos(ix+p.x(), iy+p.y());
        if (mHeightGrid->isIndexValid(pos)) {
            float &rHGrid = mHeightGrid->valueAtIndex(pos);
            if (rHGrid > mHeight) // skip calculation if grid is higher than tree
                continue;
            int direction = 4 + (ix?(ix<0?-3:3):0) + (iy?(iy<0?-1:1):0); // 4 + 3*sgn(x) + sgn(y)
            hdom = mHeight - dist[direction];
            if (ring>1)
                hdom -= (ring-1)*10;

            rHGrid = qMax(rHGrid, hdom); // write value
        } // is valid
    } // for (y)
}

double Tree::readStamp()
{
    if (!mStamp)
        return 0.;
    const Stamp *stamp = mStamp->reader();
    if (!stamp)
        return 0.;
    QPoint pos = mGrid->indexAt(mPosition);
    int offset = stamp->offset();
    pos-=QPoint(offset, offset);
    QPoint p;

    int x,y;
    double sum=0.;
    for (x=0;x<stamp->size();++x) {
        for (y=0;y<stamp->size(); ++y) {
           p = pos + QPoint(x,y);
           if (mGrid->isIndexValid(p))
               sum += mGrid->valueAtIndex(p) * (*stamp)(x,y);
        }
    }
    float eigenvalue = mStamp->readSum() * lafactor;
    mLRI = sum - eigenvalue;// additive
    float dom_height = (*mHeightGrid)[mPosition];
    if (dom_height>0.)
       mLRI = mLRI / dom_height;

    //mImpact = sum + eigenvalue;// multiplicative
    // read dominance field...

    if (dom_height < mHeight) {
        // if tree is higher than Z*, the dominance height
        // a part of the crown is in "full light".
        // total value = zstar/treeheight*value + 1-zstar/height
        // reformulated to:
        mLRI =  mLRI * dom_height/mHeight ;
        m_statAboveZ++;
    }
    if (fabs(mLRI < 0.000001))
        mLRI = 0.f;
    qDebug() << "Tree #"<< id() << "value" << sum << "eigenvalue" << eigenvalue << "Impact" << mLRI;
    return mLRI;
}

/*
double Tree::readStampMul()
{
    if (!m_stamp)
        return 0.;
    const Stamp *reader = m_stamp->reader();
    if (!reader)
        return 0.;
    QPoint pos_reader = m_grid->indexAt(m_Position);

    int offset_reader = reader->offset();
    int offset_writer = m_stamp->offset();
    int d_offset = offset_writer - offset_reader;

    QPoint pos_writer=pos_reader - QPoint(offset_writer, offset_writer);
    pos_reader-=QPoint(offset_reader, offset_reader);
    QPoint p;

    float dom_height = (*m_dominanceGrid)[m_Position];
    float local_dom;

    int x,y;
    double sum=0.;
    double value, own_value;
    for (x=0;x<reader->size();++x) {
        for (y=0;y<reader->size(); ++y) {
            p = pos_reader + QPoint(x,y);
            if (m_grid->isIndexValid(p)) {
                local_dom = m_dominanceGrid->valueAt( m_grid->cellCoordinates(p) );
                own_value = 1. - m_stamp->offsetValue(x,y,d_offset)*lafactor /local_dom; // old: dom_height;
                own_value = qMax(own_value, 0.02);
                value =  m_grid->valueAtIndex(p) / own_value; // remove impact of focal tree
                if (value>0.)
                    sum += value * (*reader)(x,y);
                //value = (1. - m_stamp->offsetValue(x,y,d_offset)/dom_height);
                //value = (1 - m_grid->valueAtIndex(p)) / (m_stamp->offsetValue(x,y,d_offset)/dom_height * lafactor);
                if (isDebugging()) {
                    qDebug() << "Tree" << id() << "Coord" << p << "gridvalue" << m_grid->valueAtIndex(p) << "value" << value << "reader" << (*reader)(x,y) << "writer" << m_stamp->offsetValue(x,y,d_offset);
                }

            }
        }
    }
    mImpact = sum;
    // read dominance field...
    if (dom_height < m_Height) {
        // if tree is higher than Z*, the dominance height
        // a part of the crown is in "full light".
        m_statAboveZ++;
        mImpact = 1. - (1. - mImpact)*dom_height/m_Height;
    }
    if (mImpact > 1)
        mImpact = 1.f;
    //qDebug() << "Tree #"<< id() << "value" << sum << "Impact" << mImpact;
    return mImpact;
}*/

void Tree::readStampMul()
{
    if (!mStamp)
        return;
    const Stamp *reader = mStamp->reader();
    if (!reader)
        return;
    QPoint pos_reader = mGrid->indexAt(mPosition);

    int offset_reader = reader->offset();
    int offset_writer = mStamp->offset();
    int d_offset = offset_writer - offset_reader; // offset on the *stamp* to the crown-cells

    QPoint pos_writer=pos_reader - QPoint(offset_writer, offset_writer);
    pos_reader-=QPoint(offset_reader, offset_reader);
    QPoint p;

    //float dom_height = (*m_dominanceGrid)[m_Position];
    float local_dom;

    int x,y;
    double sum=0.;
    double value, own_value;
    float *grid_value;
    int reader_size = reader->size();
    int rx = pos_reader.x();
    int ry = pos_reader.y();
    for (y=0;y<reader_size; ++y, ++ry) {
        grid_value = mGrid->ptr(rx, ry);
        for (x=0;x<reader_size;++x) {

            //p = pos_reader + QPoint(x,y);
            //if (m_grid->isIndexValid(p)) {
            local_dom = mHeightGrid->valueAtIndex((rx+x)/5, ry/5);
            //local_dom = m_dominanceGrid->valueAt( m_grid->cellCoordinates(p) );
            own_value = 1. - mStamp->offsetValue(x,y,d_offset)*lafactor /local_dom; // old: dom_height;
            own_value = qMax(own_value, 0.02);
            value =  *grid_value++ / own_value; // remove impact of focal tree
            //if (value>0.)
            sum += value * (*reader)(x,y);

            //} // isIndexValid
        }
    }
    mLRI = sum;
    // read dominance field...
    // this applies only if some trees are potentially *higher* than the dominant height grid
    //if (dom_height < m_Height) {
        // if tree is higher than Z*, the dominance height
        // a part of the crown is in "full light".
    //    m_statAboveZ++;
    //    mImpact = 1. - (1. - mImpact)*dom_height/m_Height;
    //}
    if (mLRI > 1)
        mLRI = 1.f;
    //qDebug() << "Tree #"<< id() << "value" << sum << "Impact" << mImpact;
    mRU->addWLA(mLRI * mLeafArea, mLeafArea);
}

void Tree::resetStatistics()
{
    m_statPrint=0;
    m_statCreated=0;
    m_statAboveZ=0;
    m_nextId=1;
}

//////////////////////////////////////////////////
////  Growth Functions
//////////////////////////////////////////////////

/// evaluate allometries and calculate LeafArea
void Tree::calcBiomassCompartments()
{
    mLeafMass = mSpecies->biomassLeaf(mDbh);
    mRootMass = mSpecies->biomassRoot(mDbh);
    mStemMass = mSpecies->biomassStem(mDbh);
    // LeafArea[m2] = LeafMass[kg] * specificLeafArea[m2/kg]
    mLeafArea = mLeafMass * mSpecies->specificLeafArea();
}


void Tree::grow()
{
    // step 1: get radiation from ressource unit: radiaton (MJ/tree/year) total intercepted radiation for this tree per year!
    double radiation = mRU->interceptedRadiation(mLRI * mLeafArea);
    // step 2: get fraction of PARutilized, i.e. fraction of intercepted rad that is utiliziable (per year)

    double raw_gpp_per_rad = mRU->ressourceUnitSpecies(mSpecies).prod3PG().GPPperRad();
    // GPP (without aging-effect) [gC] / year
    double raw_gpp = raw_gpp_per_rad * radiation;
    /*
    if (mRU->index()==3) {
        qDebug() << "tree production: radiation: " << radiation << "gpp/rad:" << raw_gpp_per_rad << "gpp" << raw_gpp << "LRI:" << mLRI << "LeafArea:" << mLeafArea;
    }*/
    // apply aging
    double gpp = raw_gpp * 0.6; // aging
    double npp = gpp * 0.47; // respiration loss

    partitioning(npp);

    calcBiomassCompartments();

}

void Tree::partitioning(double npp)
{
    double harshness = mRU->ressourceUnitSpecies(mSpecies).prod3PG().harshness();
    // add content of reserve pool
    npp += mNPPReserve;

}


