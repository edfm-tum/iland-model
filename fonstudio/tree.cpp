
#include "tree.h"
#include "core/grid.h"
#include "imagestamp.h"

#include "core/stamp.h"
#include "treespecies.h"

Expression Tree::rScale=Expression();
Expression Tree::hScale=Expression();
FloatGrid *Tree::m_grid = 0;
FloatGrid *Tree::m_dominanceGrid = 0;
int Tree::m_statPrint=0;
int Tree::m_statAboveZ=0;
int Tree::m_nextId=0;
float Tree::lafactor = 1.;
int Tree::m_debugid = -1;

Tree::Tree()
{
    m_Dbh = 0;
    m_Height = 0;
    m_species = 0;
    m_id = m_nextId++;
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

void Tree::stampOnGrid(ImageStamp& stamp, FloatGrid& grid)
{

    // use formulas to derive scaling values...
    rScale.setVar("height", m_Height);
    rScale.setVar("dbh", m_Dbh);
    hScale.setVar("height", m_Height);
    hScale.setVar("dbh", m_Dbh);

    double r = rScale.execute();
    double h = hScale.execute();
    stamp.setScale(r, h); // stamp uses scaling values to calculate impact
    mImpactRadius = r;

    float cell_value, r_cell, phi_cell;
    QPoint ul = grid.indexAt(QPointF(m_Position.x()-r, m_Position.y()-r) );
    QPoint lr =  grid.indexAt( QPointF(m_Position.x()+r, m_Position.y()+r) );
    QPoint centercell=grid.indexAt(position());
    grid.validate(ul); grid.validate(lr);
    QPoint cell;
    QPointF cellcoord;
    int ix, iy;
    mOwnImpact=0.f;
    mImpactArea=0.f;
    for (ix=ul.x(); ix<lr.x(); ix++)
        for (iy=ul.y(); iy<lr.y(); iy++) {
        cell.setX(ix); cell.setY(iy);
        cellcoord = grid.cellCoordinates(cell);
        r_cell = dist_and_direction(m_Position, cellcoord, phi_cell);
        if (r_cell > r && cell!=centercell)
            continue;
        // get value from stamp at this location (given by radius and angle)
        cell_value = stamp.get(r_cell, phi_cell);
        // add value to cell
        mOwnImpact+=(1. - cell_value);
        mImpactArea++;
        grid.valueAtIndex(cell)*=cell_value;
    }
}

float Tree::retrieveValue(ImageStamp& stamp, FloatGrid& grid)
{

    rScale.setVar("height", m_Height);
    rScale.setVar("dbh", m_Dbh);
    hScale.setVar("height", m_Height);
    hScale.setVar("dbh", m_Dbh);
    double r = rScale.execute();
    double h = hScale.execute();
    stamp.setScale(r, h); // stamp uses scaling values to calculate impact

    float stamp_value, cell_value, r_cell, phi_cell;
    QPoint ul = grid.indexAt(QPointF(m_Position.x()-r, m_Position.y()-r) );
    QPoint lr =  grid.indexAt( QPointF(m_Position.x()+r, m_Position.y()+r) );
    QPoint centercell=grid.indexAt(position());
    grid.validate(ul); grid.validate(lr);
    QPoint cell;
    QPointF cellcoord;
    int ix, iy;
    float value=0.f;

    int counting_cells=0;
    for (ix=ul.x(); ix<lr.x(); ix++)
        for (iy=ul.y(); iy<lr.y(); iy++) {

        cell.setX(ix); cell.setY(iy);
        cellcoord = grid.cellCoordinates(cell);
        r_cell = dist_and_direction(m_Position, cellcoord, phi_cell);
        if (r_cell>r && cell!=centercell)
            continue;
        counting_cells++;
        // get value from stamp at this location (given by radius and angle)
        stamp_value = stamp.get(r_cell, phi_cell);
        cell_value =  grid.valueAtIndex(QPoint(ix,iy));
        if (stamp_value>0.)
           value += cell_value / stamp_value;
        // sum up values of cell
        //value += cell_value;
        //value += grid.valueAtIndex(QPoint(ix,iy)); // - cell_value;
    }
    if (counting_cells>0)
        mImpact = value / float(counting_cells);
    else
        mImpact=1;
    return mImpact;
}


void Tree::setup()
{
    if (m_Dbh<=0 || m_Height<=0)
        return;
    // check stamp
   Q_ASSERT_X(m_species!=0, "Tree::setup()", "species is NULL");
   m_stamp = m_species->stamp(m_Dbh, m_Height);
}

void Tree::applyStamp()
{
    Q_ASSERT(m_grid!=0);
    if (!m_stamp)
        return;

    QPoint pos = m_grid->indexAt(m_Position);
    int offset = m_stamp->offset();
    pos-=QPoint(offset, offset);
    QPoint p;

    float &dom = m_dominanceGrid->valueAt(m_Position);

    int x,y;
    float value;
    QPoint dbg(10,20);
    for (x=0;x<m_stamp->size();++x) {
        for (y=0;y<m_stamp->size(); ++y) {
           p = pos + QPoint(x,y);
           // debug pixel
           if (p==dbg)
               qDebug() << "#" << id() << "value;"<<(*m_stamp)(x,y)<<"domH"<<dom;

           if (m_grid->isIndexValid(p)) {
               // mulitplicative:
               //m_grid->valueAtIndex(p)*=(*m_stamp)(x,y);
               // additiv:
               // multiplicatie, v2
               value = (*m_stamp)(x,y) ;
               value = 1. - value*lafactor / dom;
               if (value<0.)
                   value=0;
               m_grid->valueAtIndex(p)*= value;
           }
        }
    }

    m_statPrint++; // count # of stamp applications...
}

void Tree::heightGrid()
{
        // height of Z*
    float &dom = m_dominanceGrid->valueAt(m_Position); // modifyable reference
    // apply height in domiance grid
    float compare_height;
    if (m_Height<15)
        compare_height = m_Height;
    //if (m_Height<10)
    //    compare_height = m_Height/2.f;
    else
        compare_height = m_Height - 5.f;
    if (dom < compare_height)
        dom =  compare_height; // set height (via ref)

}

double Tree::readStamp()
{
    if (!m_stamp)
        return 0.;
    const Stamp *stamp = m_stamp->reader();
    if (!stamp)
        return 0.;
    QPoint pos = m_grid->indexAt(m_Position);
    int offset = stamp->offset();
    pos-=QPoint(offset, offset);
    QPoint p;

    int x,y;
    double sum=0.;
    for (x=0;x<stamp->size();++x) {
        for (y=0;y<stamp->size(); ++y) {
           p = pos + QPoint(x,y);
           if (m_grid->isIndexValid(p))
               sum += m_grid->valueAtIndex(p) * (*stamp)(x,y);
        }
    }
    float eigenvalue = m_stamp->readSum() * lafactor;
    mImpact = sum - eigenvalue;// additive
    float dom_height = (*m_dominanceGrid)[m_Position];
    if (dom_height>0.)
       mImpact = mImpact / dom_height;

    //mImpact = sum + eigenvalue;// multiplicative
    // read dominance field...

    if (dom_height < m_Height) {
        // if tree is higher than Z*, the dominance height
        // a part of the crown is in "full light".
        // total value = zstar/treeheight*value + 1-zstar/height
        // reformulated to:
        mImpact =  mImpact * dom_height/m_Height ;
        m_statAboveZ++;
    }
    if (fabs(mImpact < 0.000001))
        mImpact = 0.f;
    qDebug() << "Tree #"<< id() << "value" << sum << "eigenvalue" << eigenvalue << "Impact" << mImpact;
    return mImpact;
}

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

    int x,y;
    double sum=0.;
    double value, own_value;
    for (x=0;x<reader->size();++x) {
        for (y=0;y<reader->size(); ++y) {
            p = pos_reader + QPoint(x,y);
            if (m_grid->isIndexValid(p)) {
                own_value = 1. - m_stamp->offsetValue(x,y,d_offset)*lafactor /dom_height;
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
    qDebug() << "Tree #"<< id() << "value" << sum << "Impact" << mImpact;
    return mImpact;
}


void Tree::resetStatistics()
{
    m_statPrint=0;
    m_statAboveZ=0;
    m_nextId=1;
}
