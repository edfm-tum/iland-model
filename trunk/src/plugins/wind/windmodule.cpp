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
#include "windmodule.h"
#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "helper.h"
#include "tree.h"
#include "species.h"
#include "speciesset.h"
#include "resourceunit.h"

/** @defgroup windmodule iLand windmodule
  The wind module is a disturbance module within the iLand framework.

  FIXME  !!
   See http://iland.boku.ac.at/wind for the science behind the module,
  and http://iland.boku.ac.at/fire+module for the implementation/ using side.
 */


/** @class WindModule

    FIXME some more details
  */

double WindLayers::value(const WindCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.height==9999.f?0:data.height; // height
    case 1: return data.edge; // maximum difference to neighboring cells
    case 2: return data.cws_uproot; // critical wind speed for uprooting
    case 3: return data.cws_break; // critical wind speed for stem breakage
    case 4: return (double) data.n_killed; // trees killed on pixel
    case 5: return data.basal_area_killed; // basal area killed on pixel
    case 6: return data.n_iteration; // iteration in processing that the current pixel is processed (and trees are killed)
    case 7: return data.crown_windspeed; // effective wind speed in the crown (m/s)
    default: throw IException(QString("invalid variable index for a WindCell: %1").arg(param_index));
    }
}


const QStringList WindLayers::names() const
{
    return QStringList() <<  "height" << "edge" << "cwsUproot" << "cwsBreak" << "nKilled" << "basalAreaKilled" << "iteration" << "windSpeedCrown";

}

WindModule::WindModule()
{
    mWindDirection = 0.;
    mWindSpeed = 0.;
    mSimulationMode = false;
}


void WindModule::setup()
{
    // setup the grid (using the size/resolution)
    //mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
    //              GlobalSettings::instance()->model()->RUgrid().cellsize());
    // setup the fire spread grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    //mFireId = 0;

    // set some global settings
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.wind"));
//    mWindSpeedMin = xml.valueDouble(".wind.speedMin", 5.);
//    mWindSpeedMax = xml.valueDouble(".wind.speedMax", 10.);
//    mWindDirection = xml.valueDouble(".wind.direction", 270.); // defaults to "west"
    //    mFireSizeSigma = xml.valueDouble(".fireSizeSigma", 0.25);

    mWindLayers.setGrid(mGrid);
    GlobalSettings::instance()->controller()->addLayers(&mWindLayers, "wind");

    // setup the species parameters that are specific to the wind module
    QString parameter_table_name = xml.value(".speciesParameter", "wind");
    loadSpeciesParameter(parameter_table_name);
}

void WindModule::loadSpeciesParameter(const QString &table_name)
{
    QSqlQuery query(GlobalSettings::instance()->dbin());
    QString sql = QString("select * from %1").arg(table_name);
    query.exec(sql);
    if (query.lastError().isValid()){
        throw IException(QString("Error loading species parameters for wind module: %1 \n %2").arg(sql, query.lastError().text()) );
    }
    mSpeciesParameters.clear();
    int iID=query.record().indexOf("shortName");
    int iCreg = query.record().indexOf("CReg");
    int iCrownArea = query.record().indexOf("crownAreaFactor");
    int iCrownLength = query.record().indexOf("crownLength");
    int iMOR = query.record().indexOf("MOR");
    int iWet = query.record().indexOf("wetBiomassFactor");
    if (iID==-1 || iCreg==-1 || iCrownArea==-1 || iCrownLength==-1 || iMOR==-1 || iWet==-1) {
        throw IException(QString("Error in wind parameter table '%1'. A required column was not found.").arg(table_name));
    }
    QString species_id;
    while (query.next()) {
        species_id = query.value(iID).toString();
        Species *s = GlobalSettings::instance()->model()->speciesSet()->species(species_id);
        if (s) {
            WindSpeciesParameters &p = mSpeciesParameters[s];
            p.Creg = query.value(iCreg).toDouble();
            p.crown_area_factor = query.value(iCrownArea).toDouble();
            p.crown_length = query.value(iCrownLength).toDouble();
            p.MOR = query.value(iMOR).toDouble();
            p.wet_biomass_factor = query.value(iWet).toDouble();
        }
    }
    qDebug() << "wind:"  << mSpeciesParameters.count() << "species parameter vectors loaded.";
}

const WindSpeciesParameters &WindModule::speciesParameter(const Species *s)
{
    if (!mSpeciesParameters.contains(s))
        throw IException(QString("WindModule: no wind species parameter for species '%1'").arg(s->id()));
    return mSpeciesParameters[s];
}

/// the run() function executes the fire events
void WindModule::run()
{
    // (1) do we have a wind event this year?

    // (2) do the actual wind calculations
    // (2.1) setup the wind data
    initWindGrid();

    int total_pixels = 0;
    bool finished = false;
    mCurrentIteration = 1;
    DebugTimer ttotal("wind:total");
    while (!finished) {
        // check for edges in the stand
        DebugTimer titeration("wind:Cycle");
        // detect current edges in the forest
        detectEdges();
        // calculate the gap sizes/fetch for the current structure
        calculateFetch();
        // derive the impact of wind (i.e. calculate critical wind speeds and the effect of wind on the forest)
        int pixels = calculateWindImpact();
        if (pixels == 0)
            finished = true; // nothing found
        total_pixels += pixels;
        if (++mCurrentIteration>10)
            finished = true;
        qDebug() << "wind module: iteration" << mCurrentIteration-1 << "this round:" << pixels << "total:" << total_pixels;
    }
    qDebug() << "iterations: " << mCurrentIteration << "total pixels affected:" << total_pixels;
}

void WindModule::initWindGrid()
{
    // as long as we have 10m -> easy!
    if (cellsize()==cHeightPerRU) {
        WindCell *p = mGrid.begin();
        for (const HeightGridValue *hgv=GlobalSettings::instance()->model()->heightGrid()->begin(); hgv!=GlobalSettings::instance()->model()->heightGrid()->end(); ++hgv, ++p) {
            p->clear();
            p->height = hgv->isValid()?hgv->height:9999.f;
        }
        return;
    }
    throw IException("WindModule::initWindGrid: not 10m of windpixels...");
}


void WindModule::detectEdges()
{
    WindCell *p_above, *p, *p_below;
    int dy = mGrid.sizeY();
    int dx = mGrid.sizeX();
    int x,y;
    const float threshold = 10.f;
    for (y=1;y<dy-1;++y){
        p = mGrid.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            p->edge = 0.f; // no edge
            if (p->isValid()) {
                float max_h = p->height - threshold; // max_h: if a surrounding pixel is higher than this value, then there is no edge here
                if (max_h > 0) {
                    // check 8-neighborhood. if one of those pixels exceed the threshold, then the current
                    // pixel is not an "edge".
                    if (((p_above-1)->height < max_h) ||
                            ((p_above)->height < max_h) ||
                            ((p_above+1)->height < max_h) ||
                            ((p-1)->height < max_h)  ||
                            ((p+1)->height < max_h) ||
                            ((p_below-1)->height < max_h) ||
                            ((p_below)->height < max_h) ||
                            ((p_below+1)->height < max_h) ) {
                        // edge found:
                        p->edge = 1.f;
                    }
                }
            }
        }
    }
}

void WindModule::calculateFetch()
{
    int calculated = 0;

    WindCell *end = mGrid.end();
    for (WindCell *p=mGrid.begin(); p!=end; ++p) {
        if (p->edge == 1.f) {
            QPoint pt=mGrid.indexOf(p);
            checkFetch(pt.x(), pt.y(), mWindDirection, p->height * 10., p->height - 10.);
            ++calculated;
        }
   }
    qDebug() << "calculated fetch for" << calculated << "pixels";
}

/** calculate for each pixel the impact of wind
  @return number of pixels with killed trees
  */
int WindModule::calculateWindImpact()
{
    int calculated = 0;
    int effective = 0;
    WindCell *end = mGrid.end();
    for (WindCell *p=mGrid.begin(); p!=end; ++p) {
        if (p->edge >= 1.f) {
            QPoint pt=mGrid.indexOf(p);
            if (windImpactOnPixel(pt, p))
                ++effective;
            ++calculated;
        }
    }
    qDebug() << "calculated fetch for" << calculated << "pixels";
    return effective;
}



// test function
void WindModule::testFetch(double degree_direction)
{
//    for (int i=0;i<350;i+=10) {
//        checkFetch(100,100, i*M_PI/180., 500+i);
//    }
    double direction = degree_direction*M_PI/180.;
    int calculated = 0;

    WindCell *end = mGrid.end();
    for (WindCell *p=mGrid.begin(); p!=end; ++p) {
        if (p->edge == 1.f) {
            QPoint pt=mGrid.indexOf(p);
            checkFetch(pt.x(), pt.y(), direction, p->height * 10., p->height - 10.);
            ++calculated;
        }
   }
    qDebug() << "calculated fetch for" << calculated << "pixels";
}

void WindModule::testEffect()
{
    int calculated = 0;
    WindCell *end = mGrid.end();
    for (WindCell *p=mGrid.begin(); p!=end; ++p) {
        if (p->edge >= 1.f) {
            QPoint pt=mGrid.indexOf(p);
            windImpactOnPixel(pt, p);
            ++calculated;
        }
    }
    qDebug() << "calculated fetch for" << calculated << "pixels";
}

/** find distance to the next shelter pixel
  @param startx x-index of the starting pixel (index)
  @param starty y-index of the starting pixel (index)
  @param direction direction to look (rad, 0: north, pi/2: east, pi: south, 3/2pi: west)
  @param max_distance maximum distance (meters) to look for
  @param threshold algorithm terminates if a pixel with a height higher than threshold is found
  */
bool WindModule::checkFetch(const int startx, const int starty, const double direction, const double max_distance, const double threshold)
{
    int endx = startx + (max_distance/cellsize()+0.5)*sin(direction);
    int endy = starty + (max_distance/cellsize()+0.5)*cos(direction);

    // "draw" a line using bresenhems algorithm (http://en.wikipedia.org/wiki/Bresenham's_line_algorithm)
    int dx = abs(endx-startx);
    int dy = abs(endy-starty);
    int sx = startx<endx?1:-1;
    int sy = starty<endy?1:-1;
    int err = dx - dy;

    int x = startx,y = starty;
    while (true) {
        if (mGrid.isIndexValid(x,y)) {
            if ((x!=startx || y!=starty) && (mGrid.valueAtIndex(x,y).height > threshold)) {
                double dist = sqrt(cellsize()*((x-startx)*(x-startx) + (y-starty)*(y-starty)));
                mGrid.valueAtIndex(startx, starty).edge = dist;
                return true;
            }
            // mGrid.valueAtIndex(x,y).edge = 10; // plot...
        } else
            break;
        if (x==endx && y==endy)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    mGrid.valueAtIndex(startx, starty).edge = max_distance;
    return false;
/*
function line(x0, y0, x1, y1)
   dx := abs(x1-x0)
   dy := abs(y1-y0)
   if x0 < x1 then sx := 1 else sx := -1
   if y0 < y1 then sy := 1 else sy := -1
   err := dx-dy

   loop
     setPixel(x0,y0)
     if x0 = x1 and y0 = y1 exit loop
     e2 := 2*err
     if e2 > -dy then
       err := err - dy
       x0 := x0 + sx
     end if
     if e2 <  dx then
       err := err + dx
       y0 := y0 + sy
     end if
   end loop
*/
}




/** Main function of the wind module
  @param position the integer index (x/y) of the grid cell for which the wind effect should be calculated
  @param cell the content of the current wind cell (for convenience)
    @return true, if trees were killed (thrown, broken)
  */
bool WindModule::windImpactOnPixel(const QPoint position, WindCell *cell)
{
    // extract a list of trees that are within the pixel boundaries
    QRectF pixel_rect = mGrid.cellRect(position);
    ResourceUnit *ru = GlobalSettings::instance()->model()->ru(pixel_rect.center());
    if (!ru)
        return false;

    // ************************************************
    // retrieve a list of trees within the active pixel
    // ************************************************
    // NOTE: the check with isDead() is necessary because dead trees could be already in the trees list
    double h_max = 0.;
    QVector<Tree*> trees;
    Tree *tallest_tree = 0;
    QVector<Tree>::iterator tend = ru->trees().end();
    for (QVector<Tree>::iterator t = ru->trees().begin(); t!=tend; ++t) {
        if ( pixel_rect.contains( (*t).position() ) && !(*t).isDead()) {
            trees.push_back(&(*t));
            if (trees.last()->height()> h_max) {
                tallest_tree = trees.last();
                h_max = tallest_tree->height();
            }
        }
    }

    // no trees on the cell -> do nothing
    if (trees.isEmpty()) {
        // this can happen, if very large trees have crowns that cover more than one height pixel
        cell->height = 0.f;
        cell->edge = 0.f;
        return false;
    }

    int n_trees = trees.size();

    // *****************************************************************************
    // Calculate the wind speed at the crown top and the critical wind speeds
    // *****************************************************************************
    const WindSpeciesParameters &params = speciesParameter(tallest_tree->species());
    double wind_speed_10 = mWindSpeed; // wind speed on current resource unit 10m above the canopy TODO!!
    double u_crown = calculateCrownWindSpeed(tallest_tree, params, n_trees, wind_speed_10);
    double cws_uproot, cws_break;
    calculateCrititalWindSpeed(tallest_tree, params, cell->edge, cws_uproot, cws_break);
    cell->cws_break = cws_break;
    cell->cws_uproot = cws_uproot;
    cell->crown_windspeed = u_crown;

    if (u_crown < cws_uproot && u_crown < cws_break)
        return false; // wind speed is too low

    // whether uprooting or breaking occurs depend on the wind speed: stems break if speed is high enough, otherwise uprooting will occur
    bool do_break = u_crown > cws_break;

    // *****************************************************************************
    // Kill the trees that are thrown/uprooted by the wind
    // *****************************************************************************
    foreach(Tree *tree, trees) {
        if (!mSimulationMode) {
            // all trees > 4m are killed on the cell
            if (do_break) {
                // breaking
                // half of the stem as well as foliage/branches are moved to the soil. The other half
                // of the stem remains as snag.
                // TODO!!!!


            } else {
                // uprooting
                // all biomass is moved to the soil
                // TODO!
                // regeneration is killed in case of uprooting
                ru->clearSaplings(pixel_rect, true);

            }
            tree->die();
        }
        // statistics
        cell->basal_area_killed += tree->basalArea();
        cell->n_killed ++;

    }

    // reset the current cell
    cell->height = 0.f;
    cell->edge = 0.f;
    cell->n_iteration = mCurrentIteration;
    return true;
}

/** calculate the windspeed at the top of the crown.

  @param tree the tallest tree on the cell
  @param n_trees number of trees that are on the 10x10m cell
  @param wind_speed_10 wind speed in 10m above canopy (m/s)
  */
double WindModule::calculateCrownWindSpeed(const Tree *tree, const WindSpeciesParameters &params, const int n_trees, const double wind_speed_10)
{
     // frontal area index
    double lambda = 2. * tree->crownRadius() * (tree->height() * params.crown_length )* params.crown_area_factor / (cellsize()*cellsize()/n_trees);
    // calculate zero-plane-displacement height (Raupachs drag partitioning model (Raupach 1992, 1994))
    const double cdl = 7.5;
    double d0_help = sqrt(2. * cdl * lambda);
    double d0 = tree->height() * (1. - (1.-exp(-d0_help))/d0_help);

    const double surface_drag_coefficient = 0.003;
    const double element_drag_coefficient = 0.3;
    const double kaman_constant = 0.4;

    // drag coefficient gamma
    double gamma = sqrt(surface_drag_coefficient + lambda*element_drag_coefficient);
    if (gamma > element_drag_coefficient) gamma = element_drag_coefficient;

    // surface roughness
    double z0 = (tree->height() - d0)*exp(-kaman_constant/gamma + 0.193);

    // wind multiplier
    double u_factor = log((tree->height()-d0)/z0) / log((tree->height()+10.)/z0);
    double u_crown = wind_speed_10 * u_factor;

    return u_crown;
}

/** calculate the critital windspeed taking into account the sheltering from upwind vegetation and the competetive state of the tree.
  The calculation is performed for the largest tree on the cell.
  @param tree the tallest tree on the cell
  @param gap_length size of the gap in upwind direction (m)
  @param rCWS_uproot OUT parameter with the critical windspeed for uprooting
  @param rCWS_uproot OUT parameter with the critical windspeed for stem breakage
  @return the (lower) critital windspeed

  */
double WindModule::calculateCrititalWindSpeed(const Tree *tree, const WindSpeciesParameters &params, const double gap_length, double &rCWS_uproot, double &rCWS_break)
{
    // relate the gap size to tree length and calcualte the f_gap factor
    double rel_gap = gap_length / tree->height();
    if (rel_gap>10) rel_gap=10;

    double f_gap = (0.001+0.001*pow(rel_gap,0.562))/0.00465; // formulation from Peltola et al. (1999), based on Gardiner et al. (1997)

    // calculate the wet stem weight (iLand internally uses always dry weights)
    double stem_mass = tree->biomassStem() * params.wet_biomass_factor;

    // calculate the "BAL", the basal area of surrounding trees, i.e. a competition index
    // this will be changed to be related to LRI. But for the time being:
    double bal = 70. * (1. - tree->lightResourceIndex());

    // the turning moment coefficient incorporating the competition state
    double tc = 36.8+112.2*(tree->dbh()*tree->dbh()/10000.)*tree->height()-0.460*bal-0.783*(tree->dbh()*tree->dbh()/10000.)*tree->height()*bal;
    // now derive the critital wind speeds for uprooting and breakage
    const double f_knot = 1.; // a reduction factor accounting for the presence of knots, currenty no reduction.

    rCWS_uproot = sqrt((params.Creg*stem_mass) / (tc*f_gap)); // critical windspeed for uprooting
    rCWS_break = sqrt(params.MOR*pow(tree->dbh(),3)*f_knot*M_PI / (32.*tc*f_gap)); // critical windspeed for breakage

    // debug info
    // qDebug() << "f_gap, bal, tc, cws_uproot, cws_break:" << f_gap << bal << tc << rCWS_uproot << rCWS_break;
    return qMin(rCWS_uproot, rCWS_break);
}

