#include "global.h"
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

/// dumps some core variables of a tree to a string.
QString Tree::dump()
{
    QString result = QString("id %1 species %2 dbh %3 h %4 x/y %5/%6 ru# %7 LRI %8")
                            .arg(mId).arg(mSpecies->id()).arg(mDbh).arg(mHeight)
                            .arg(mPosition.x()).arg(mPosition.y())
                            .arg(mRU->index()).arg(mLRI);
    return result;
}

void Tree::dumpList(DebugList &rTargetList)
{
    rTargetList << mId << mSpecies->id() << mDbh << mHeight  << mPosition.x() << mPosition.y()   << mRU->index() << mLRI
                << mStemMass << mRootMass << mLeafMass << mLeafArea;
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
   mDbhDelta = 0.1; // initial value: used in growth() to estimate diameter increment

}

//////////////////////////////////////////////////
////  Light functions (Pattern-stuff)
//////////////////////////////////////////////////

#define NOFULLDBG
//#define NOFULLOPT


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
            local_dom = mHeightGrid->valueAtIndex((rx+x)/5, ry/5); // ry: gets ++ed in outer loop, rx not
            //local_dom = m_dominanceGrid->valueAt( m_grid->cellCoordinates(p) );

            own_value = 1. - mStamp->offsetValue(x,y,d_offset)*lafactor / local_dom; // old: dom_height;
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

     mStamp = mSpecies->stamp(mDbh, mHeight); // get new stamp for updated dimensions

}


// just used to test the DBG_IF_x macros...
QString test_cntr()
{
    static int cnt = 0;
    cnt++;
    return QString::number(cnt);
}

#if !defined(DBGMODE)
#  ifndef QT_NO_DEBUG
#    define DBGMODE(stmts) { stmts }
#  else
#    define DBGMODE(stmts) qt_noop()
#  endif
#endif

void Tree::partitioning(double npp)
{
    DBGMODE(
        if (mId==1)
            test_cntr();
    );
    double harshness = mRU->ressourceUnitSpecies(mSpecies).prod3PG().harshness();
    // add content of reserve pool
    npp += mNPPReserve;
    const double reserve_size = mLeafMass;

    double apct_wood, apct_root, apct_foliage; // allocation percentages (sum=1)
    // turnover rates
    const double &to_fol = mSpecies->turnoverLeaf();
    const double &to_wood = mSpecies->turnoverStem();
    const double &to_root = mSpecies->turnoverRoot();

    apct_root = harshness;

    double b_wf = 1.32; // ratio of allometric exponents... now fixed

    // Duursma 2007, Eq. (20)
    apct_wood = (mLeafMass * to_wood / npp + b_wf*(1.-apct_root) - b_wf * to_fol/npp) / ( mStemMass / mStemMass + b_wf );
    apct_foliage = 1. - apct_root - apct_wood;

    // senescence demands of the compartemnts
    double senescence_foliage = mLeafMass * to_fol;
    double senescence_stem = mStemMass * to_wood;
    double senescence_root = mRootMass * to_root;

    // brutto compartment increments
    double gross_foliage = npp * apct_foliage;
    double gross_stem = npp * apct_wood;
    double gross_root = npp * apct_root;

    // netto increments
    double net_foliage = gross_foliage - senescence_foliage;
    double net_root = gross_root - senescence_root;
    double net_stem = gross_stem - senescence_stem;

    // flow back to reserve pool:
    double to_reserve = qMin(reserve_size, net_stem);
    net_stem -= to_reserve;

    /*DBG_IF_X(mId == 1 , "Tree::partitioning", "dump", dump()
             + QString("npp %1 npp_reserve %9 sen_fol %2 sen_stem %3 sen_root %4 net_fol %5 net_stem %6 net_root %7 to_reserve %8")
               .arg(npp).arg(senescence_foliage).arg(senescence_stem).arg(senescence_root)
               .arg(net_foliage).arg(net_stem).arg(net_root).arg(to_reserve).arg(mNPPReserve) );*/

    DBGMODE(
        if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreePartition) && mId<300) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreePartition);
            dumpList(out); // add tree headers
            out << npp << senescence_foliage << senescence_stem << senescence_root << net_foliage << net_stem << net_root << to_reserve << mNPPReserve;
        }
    ); // DBGMODE()

    // update of compartments
    mLeafMass += net_foliage;
    mLeafMass = qMax(mLeafMass, 0.f);
    mLeafArea = mLeafMass * mSpecies->specificLeafArea();

    mRootMass += net_root;
    mRootMass = qMax(mRootMass, 0.f);

    // calculate the dimensions of growth (diameter, height)
    grow_diameter(net_stem);

    // calculate stem biomass using the allometric equation
    mStemMass = mSpecies->biomassStem(mDbh);

}


/** Determination of diamter and height growth based on increment of the stem mass (@net_stem_npp).
    Refer to XXX for equations and variables.
    This function updates the dbh and height of the tree.
  */
inline void Tree::grow_diameter(const double &net_stem_npp)
{
    // determine dh-ratio of increment
    // height increment is a function of light competition:
    double hd_growth = relative_height_growth(); // hd of height growth
    //DBG_IF_X(rel_height_growth<0 || rel_height_growth>1., "Tree::grow_dimater", "rel_height_growth out of bound.", dump());

    const double volume_factor = mSpecies->volumeFactor();

    double factor_diameter = 1. / (  volume_factor * (mDbh + mDbhDelta)*(mDbh + mDbhDelta) * ( 2. * mHeight/mDbh + hd_growth) );
    double delta_d_estimate = factor_diameter * net_stem_npp; // estimated dbh-inc using last years increment

    // using that dbh-increment we estimate a stem-mass-increment and the residual (Eq. 9)
    double stem_estimate = volume_factor * (mDbh + delta_d_estimate)*(mDbh + delta_d_estimate)*(mHeight + delta_d_estimate*hd_growth);
    double stem_residual = stem_estimate - (mStemMass + net_stem_npp);

    // the final increment is then:
    double d_increment = factor_diameter * (net_stem_npp - stem_residual); // Eq. (11)
    DBG_IF_X(mId == 1 || d_increment<0., "Tree::grow_dimater", "increment < 0.", dump()
             + QString("\nhdz %1 factor_diameter %2 stem_residual %3 delta_d_estimate %4 d_increment %5 final residual(kg) %6")
               .arg(hd_growth).arg(factor_diameter).arg(stem_residual).arg(delta_d_estimate).arg(d_increment)
               .arg( volume_factor * (mDbh + d_increment)*(mDbh + d_increment)*(mHeight + d_increment*hd_growth)-((mStemMass + net_stem_npp)) ));

    DBGMODE(
        double res_final = volume_factor * (mDbh + d_increment)*(mDbh + d_increment)*(mHeight + d_increment*hd_growth)-((mStemMass + net_stem_npp));
        DBG_IF_X(res_final > 1, "Tree::grow_diameter", "final residual stem estimate > 1kg", dump());
        DBG_IF_X(d_increment > 10. || d_increment*hd_growth/100. >10., "Tree::grow_diameter", "growth out of bound:",QString("d-increment %1 h-increment %2 ").arg(d_increment).arg(d_increment*hd_growth/100.) + dump());
        //dbgstruct["sen_demand"]=sen_demand;
        if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dTreeGrowth) && mId<300) {
            DebugList &out = GlobalSettings::instance()->debugList(mId, GlobalSettings::dTreeGrowth);
            dumpList(out); // add tree headers
            out << net_stem_npp << hd_growth << factor_diameter << delta_d_estimate << d_increment;
        }

            ); // DBGMODE()
    d_increment = qMax(d_increment, 0.);

    // update state variables
    mDbh += d_increment;
    mDbhDelta = d_increment; // save for next year's growth
    mHeight += d_increment * hd_growth * 0.01;
}


/// return the HD ratio of this year's increment based on the light status.
inline double Tree::relative_height_growth()
{
    double hd_low, hd_high;
    mSpecies->hdRange(mDbh, hd_low, hd_high);

    DBG_IF_X(hd_low>hd_high, "Tree::relative_height_growth", "hd low higher dann hd_high for ", dump());
    DBG_IF_X(hd_low < 20 || hd_high>250, "Tree::relative_height_growth", "hd out of range ", dump() + QString(" hd-low: %1 hd-high: %2").arg(hd_low).arg(hd_high));

    // scale according to LRI: if receiving much light (LRI=1), the result is hd_low (for open grown trees)
    double hd_ratio = hd_high - (hd_high-hd_low)*mLRI;
    return hd_ratio;
}
