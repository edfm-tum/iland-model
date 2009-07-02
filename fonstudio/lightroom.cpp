#include "lightroom.h"
#include "tools/helper.h"

LightRoom::LightRoom()
{
    m_roomObject = 0;
}

/** setup routine.
  sets up datastructures (3d space, hemi grids)
  @param size_x count of cells in x direction
  @param size_y count of cells in y direction
  @param size_z count of cells in z direction
  @param cellsize metric length of cells (used for all 3 dimensions).
  @param hemigridsize size of hemigrid-cells (in degrees).
  @param latitude lat. in degrees.
  @param diffus_frac fraction [0..1] of diffus radiation of global radiation. */
void LightRoom::setup(const int size_x, const int size_y, const int size_z,
                      const double cellsize, const double hemigridsize,
                      const double latitude, const double diffus_frac)
{
    DebugTimer t1("setup of lightroom");
    m_countX = size_x; m_countY=size_y; m_countZ=size_z;
    m_cellsize = cellsize;
    if (m_countX%2==0) m_countX++; // make uneven
    if (m_countY%2==0) m_countY++;
    QRectF rect(-m_countX/2*cellsize, -m_countY/2*cellsize, m_countX*cellsize, m_countY*cellsize);
    m_2dvalues.setup(rect, cellsize);
    m_2dvalues.initialize(0.);
    m_3dvalues.setup(rect, cellsize);
    // setup room
    int x,y;
    for (x=0;x<m_countX;x++)
        for (y=0;y<m_countY;y++)
            m_3dvalues.valueAtIndex(QPoint(x,y)).resize(size_z);
    // setup hemigrids
    SolarRadiation solar;
    solar.setLatidude(latitude); // austria
    solar.setVegetationPeriod(0,367); // no. veg. period
    solar.setDiffusRadFraction(diffus_frac); // 50% diffuse radiation
    // calculate global radiation values
    DebugTimer t2("calculate solar radiation matrix");
    solar.calculateRadMatrix(hemigridsize, m_solarGrid);
    t2.showElapsed();
    m_shadowGrid.setup(hemigridsize); // setup size
}

double LightRoom::calculateGridAtPoint(const double p_x, const double p_y, const double p_z)
{
    if (!m_roomObject)
        return 0;
    // check feasibility
    if (m_roomObject->noHitGuaranteed(p_x, p_y, p_z)) {
        qDebug()<<"skipped";
        return 0;
    }

    m_shadowGrid.clear(0.);

    // start with 45°
    int ie = m_shadowGrid.indexElevation(RAD(45));
    int ia = 0;
    int max_a = m_shadowGrid.matrixCountAzimuth();
    int max_e = m_shadowGrid.matrixCountElevation();
    double elevation, azimuth;
    bool hit;
    int c_hit = 0;
    int c_test = 0;
    for (;ie<max_e;ie++){
        for (ia=0;ia<max_a;ia++) {
            azimuth = m_shadowGrid.azimuth(ia);
            elevation = m_shadowGrid.elevation(ie);
            hit = m_roomObject->hittest(p_x, p_y, p_z,azimuth,elevation);
            //qDebug() << "testing azimuth" << GRAD(azimuth) << "elevation" << GRAD(elevation)<<"hit"<<hit;
            c_test++;
            if (hit) {
                m_shadowGrid.rGetByIndex(ia, ie) = 1.;
                c_hit++;
            }
        }
    }
    double ratio = c_hit / double(c_test);
    qDebug() << "tested"<< c_test<<"hit count:" << c_hit<<"ratio"<<c_hit/double(c_test)<<"total sum"<<m_shadowGrid.getSum();
    return ratio; // TODO: the global radiation is not calculated!!!!!

}

void LightRoom::calculateFullGrid()
{
    QVector<float> *v = m_3dvalues.begin();
    QVector<float> *vend = m_3dvalues.end();
    int z;
    QPoint pindex;
    QPointF coord;
    double hit_ratio;
    DebugTimer t("calculate full grid");
    int c=0;
    while (v!=vend) {
        pindex = m_3dvalues.indexOf(v);
        coord = m_3dvalues.getCellCoordinates(pindex);
        for (z=0;z<m_countZ;z++) {
            hit_ratio = calculateGridAtPoint(coord.x(), coord.y(), z*m_cellsize);
            (*v)[z] = hit_ratio;
        }
        v++;
        c++;
        if (c%1000==0) {
            qDebug() << c << "processed...time: ms: " << t.elapsed();
            QCoreApplication::processEvents();
        }
    }

}

//////////////////////////////////////////////////////////
// Lightroom Object
//////////////////////////////////////////////////////////

LightRoomObject::~LightRoomObject()
{
    if (m_radiusFormula)
        delete m_radiusFormula;
}

void LightRoomObject::setuptree(const double height, const double crownheight, const QString &formula)
{
    if (m_radiusFormula)
        delete m_radiusFormula;

    m_radiusFormula = new Expression(formula);
    m_baseradius = m_radiusFormula->calculate(crownheight/height);
    m_height = height;
    m_crownheight = crownheight;
}
/** The tree is located in x/y=0/0.
*/
bool LightRoomObject::hittest(const double p_x, const double p_y, const double p_z,
                              const double azimuth_rad, const double elevation_rad)
{
    if (p_z > m_height)
        return false;
    // Test 1: does the ray (azimuth) direction hit the crown?
    double phi = atan2(-p_y, -p_x); // angle between P and the tree center
    double dist2d = sqrt(p_x*p_x + p_y*p_y); // distance at ground
    if (dist2d==0)
        return true;

    double alpha = phi - azimuth_rad; // angle between the ray and the center of the tree
    if (dist2d>m_baseradius) { // test only, if p not the crown
        double half_max_angle = asin(m_baseradius / dist2d); // maximum deviation angle from direct connection p - tree where ray hits maxradius
        if (fabs(alpha) > half_max_angle)
            return false;
    } else {
        // test if p is inside the crown
        if (p_z<=m_height && p_z>=m_crownheight) {
            double radius_hit = m_radiusFormula->calculate(p_z / m_height);
            if (radius_hit <= dist2d)
                return true;
        }
    }

    // Test 2: test if the crown-"plate" at the bottom of the crown is hit.
    if (elevation_rad>0. && p_z<m_crownheight) {
        // calc. base distance between p and the point where the height of the ray reaches the bottom of the crown:
        double r_hitbottom = dist2d; // for 90°
        if (elevation_rad < M_PI_2) {
            double d_hitbottom = (m_crownheight - p_z) / tan(elevation_rad);
            // calc. position (projected) of that hit point
            double rx = p_x + sin(azimuth_rad)*d_hitbottom;
            double ry = p_y + cos(azimuth_rad)*d_hitbottom;
            r_hitbottom = sqrt(rx*rx + ry*ry);
        }
        if (r_hitbottom <= m_baseradius)
            return true;
    }
    return false;
    // Test 3: determine height of hitting tree
//    double z_hit = p_z + dist2d*tan(elevation_rad);
//    if (z_hit > m_height || z_hit<m_crownheight)
//        return false;
//    // determine angle of crownradius in hitting height (use relative height as variable in formula!)
//    // e.g. for a simple parabola: 1-h_rel^2
//    double radius_hit = m_radiusFormula->calculate(z_hit / m_height);
//    if (radius_hit < 0)
//        return false;
//    double dist3d = sqrt(p_x*p_x + p_y*p_y + (z_hit-p_z)*(z_hit-p_z) );
//    double radius_angle = asin(radius_hit/dist3d);
//
//    if (fabs(alpha) > radius_angle)
//        return false;


    return true;

}

bool LightRoomObject::noHitGuaranteed(const double p_x, const double p_y, const double p_z)
{
    // 1. simple: compare height...
    if (p_z > m_height)
        return true;
    // 2. 45° test:
    if (p_z > m_height - sqrt(p_x*p_x + p_y*p_y)) // 45°: height = distance from tree center
        return true;

    return false;
}

