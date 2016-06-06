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

#include "seeddispersal.h"

#include "globalsettings.h"
#include "model.h"
#include "debugtimer.h"
#include "helper.h"
#include "species.h"
#ifdef ILAND_GUI
#include <QtGui/QImage>
#endif

/** @class SeedDispersal
    @ingroup core
    The class encapsulates the dispersal of seeds of one species over the whole landscape.
    The dispersal algortihm operate on grids with a 20m resolution.

    See http://iland.boku.ac.at/dispersal

  */

Grid<float> *SeedDispersal::mExternalSeedBaseMap = 0;
QHash<QString, QVector<double> > SeedDispersal::mExtSeedData;
int SeedDispersal::mExtSeedSizeX = 0;
int SeedDispersal::mExtSeedSizeY = 0;

SeedDispersal::~SeedDispersal()
{
    if (isSetup()) {

    }
}

// ************ Setup **************

/** setup of the seedmaps.
  This sets the size of the seed map and creates the seed kernel (species specific)
  */
void SeedDispersal::setup()
{
    if (!GlobalSettings::instance()->model()
        || !GlobalSettings::instance()->model()->heightGrid()
        || !mSpecies)
        return;
    mProbMode = false;

    const float seedmap_size = 20.f;
    // setup of seed map
    mSeedMap.clear();
    mSeedMap.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), seedmap_size );
    mSeedMap.initialize(0.);
    if (!mProbMode) {
        mSourceMap.setup(mSeedMap);
        mSourceMap.initialize(0.);
    }
    mExternalSeedMap.clear();
    mIndexFactor = int(seedmap_size) / cPxSize; // ratio seed grid / lip-grid:
    if (logLevelInfo()) qDebug() << "Seed map setup. Species:"<< mSpecies->id() << "kernel-size: " << mSeedMap.sizeX() << "x" << mSeedMap.sizeY() << "pixels.";

    if (mSpecies==0)
        throw IException("Setup of SeedDispersal: Species not defined.");

    if (fmod(GlobalSettings::instance()->settings().valueDouble("model.world.buffer",0),seedmap_size) != 0.)
        throw IException("SeedDispersal:setup(): The buffer (model.world.buffer) must be a integer multiple of the seed pixel size (currently 20m, e.g. 20,40,60,...)).");

    // settings
    mTM_occupancy = 1.; // is currently constant
    // copy values for the species parameters:
    mSpecies->treeMigKernel(mTM_as1, mTM_as2, mTM_ks);
    mTM_fecundity_cell = mSpecies->fecundity_m2() * seedmap_size*seedmap_size * mTM_occupancy; // scale to production for the whole cell
    mNonSeedYearFraction = mSpecies->nonSeedYearFraction();
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.settings.seedDispersal"));
    mKernelThresholdArea = xml.valueDouble(".longDistanceDispersal.thresholdArea", 0.0001);
    mKernelThresholdLDD = xml.valueDouble(".longDistanceDispersal.thresholdLDD", 0.0001);
    mLDDSeedlings = xml.valueDouble(".longDistanceDispersal.LDDSeedlings", 0.0001);
    mLDDRings = xml.valueInt(".longDistanceDispersal.rings", 4);

    mLDDSeedlings = qMax(mLDDSeedlings, static_cast<float>(mKernelThresholdArea));

    // long distance dispersal
    double ldd_area = setupLDD();

    createKernel(mKernelSeedYear, mTM_fecundity_cell, 1. - ldd_area);

    // the kernel for non seed years looks similar, but is simply linearly scaled down
    // using the species parameter NonSeedYearFraction.
    // the central pixel still gets the value of 1 (i.e. 100% probability)
    createKernel(mKernelNonSeedYear, mTM_fecundity_cell*mNonSeedYearFraction, 1. - ldd_area);

    if (mSpecies->fecunditySerotiny()>0.) {
        // an extra seed map is used for storing information related to post-fire seed rain
        mSeedMapSerotiny.clear();
        mSeedMapSerotiny.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), seedmap_size );
        mSeedMapSerotiny.initialize(0.);

        // set up the special seed kernel for post fire seed rain
        createKernel(mKernelSerotiny, mTM_fecundity_cell * mSpecies->fecunditySerotiny(),1.);
        qDebug() << "created extra seed map and serotiny seed kernel for species" << mSpecies->name() << "with fecundity factor" << mSpecies->fecunditySerotiny();
    }
    mHasPendingSerotiny = false;

    // debug info
    mDumpSeedMaps = GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.dumpSeedMapsEnabled",false);
    if (mDumpSeedMaps) {
        QString path = GlobalSettings::instance()->path( GlobalSettings::instance()->settings().value("model.settings.seedDispersal.dumpSeedMapsPath") );
        Helper::saveToTextFile(QString("%1/seedkernelYes_%2.csv").arg(path).arg(mSpecies->id()),gridToString(mKernelSeedYear));
        Helper::saveToTextFile(QString("%1/seedkernelNo_%2.csv").arg(path).arg(mSpecies->id()),gridToString(mKernelNonSeedYear));
        if (!mKernelSerotiny.isEmpty())
            Helper::saveToTextFile(QString("%1/seedkernelSerotiny_%2.csv").arg(path).arg(mSpecies->id()),gridToString(mKernelSerotiny));
    }


    // external seeds
    mHasExternalSeedInput = false;
    mExternalSeedBuffer = 0;
    mExternalSeedDirection = 0;
    mExternalSeedBackgroundInput = 0.;
    if (GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.externalSeedEnabled",false)) {
        if (GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.seedBelt.enabled",false)) {
            // external seed input specified by sectors and around the project area (seedbelt)
            setupExternalSeedsForSpecies(mSpecies);
        } else {
            // external seeds specified fixedly per cardinal direction
            // current species in list??
            mHasExternalSeedInput = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedSpecies").contains(mSpecies->id());
            QString dir = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedSource").toLower();
            // encode cardinal positions as bits: e.g: "e,w" -> 6
            mExternalSeedDirection += dir.contains("n")?1:0;
            mExternalSeedDirection += dir.contains("e")?2:0;
            mExternalSeedDirection += dir.contains("s")?4:0;
            mExternalSeedDirection += dir.contains("w")?8:0;
            QStringList buffer_list = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedBuffer").split(QRegExp("([^\\.\\w]+)"));
            int index = buffer_list.indexOf(mSpecies->id());
            if (index>=0) {
                mExternalSeedBuffer = buffer_list[index+1].toInt();
                qDebug() << "enabled special buffer for species" <<mSpecies->id() << ": distance of" << mExternalSeedBuffer << "pixels = " << mExternalSeedBuffer*20. << "m";
            }

            // background seed rain (i.e. for the full landscape), use regexp
            QStringList background_input_list = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedBackgroundInput").split(QRegExp("([^\\.\\w]+)"));
            index = background_input_list.indexOf(mSpecies->id());
            if (index>=0) {
                mExternalSeedBackgroundInput = background_input_list[index+1].toDouble();
                qDebug() << "enabled background seed input (for full area) for species" <<mSpecies->id() << ": p=" << mExternalSeedBackgroundInput;
            }

            if (mHasExternalSeedInput)
                qDebug() << "External seed input enabled for" << mSpecies->id();
        }
    }

    // setup of seed kernel
//    const int max_radius = 15; // pixels
//
//    mSeedKernel.clear();
//    mSeedKernel.setup(mSeedMap.cellsize(), 2*max_radius + 1 , 2*max_radius + 1);
//    mKernelOffset = max_radius;
//    // filling of the kernel.... for simplicity: a linear kernel
//    QPoint center = QPoint(mKernelOffset, mKernelOffset);
//    const double max_dist = max_radius * seedmap_size;
//    for (float *p=mSeedKernel.begin(); p!=mSeedKernel.end();++p) {
//        double d = mSeedKernel.distance(center, mSeedKernel.indexOf(p));
//        *p = qMax( 1. - d / max_dist, 0.);
//    }


    // randomize seed map.... set 1/3 to "filled"
    //for (int i=0;i<mSeedMap.count(); i++)
    //    mSeedMap.valueAtIndex(mSeedMap.randomPosition()) = 1.;


//    QImage img = gridToImage(mSeedMap, true, -1., 1.);
//    img.save("seedmap.png");

//    img = gridToImage(mSeedMap, true, -1., 1.);
    //    img.save("seedmap_e.png");
}

void SeedDispersal::setupExternalSeeds()
{
    mExternalSeedBaseMap = 0;
    if (!GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.seedBelt.enabled",false))
        return;

    DebugTimer t("setup of external seed maps.");
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.settings.seedDispersal.seedBelt"));
    int seedbelt_width =xml.valueInt(".width",10);
    // setup of sectors
    // setup of base map
    const float seedmap_size = 20.f;
    mExternalSeedBaseMap = new Grid<float>;
    mExternalSeedBaseMap->setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), seedmap_size );
    mExternalSeedBaseMap->initialize(0.);
    if (mExternalSeedBaseMap->count()*4 != GlobalSettings::instance()->model()->heightGrid()->count())
        throw IException("error in setting up external seeds: the width and height of the project area need to be a multiple of 20m when external seeds are enabled.");
    // make a copy of the 10m height grid in lower resolution and mark pixels that are forested and outside of
    // the project area.
    for (int y=0;y<mExternalSeedBaseMap->sizeY();y++)
        for (int x=0;x<mExternalSeedBaseMap->sizeX();x++) {
            bool val = GlobalSettings::instance()->model()->heightGrid()->valueAtIndex(x*2,y*2).isForestOutside();
            mExternalSeedBaseMap->valueAtIndex(x,y) = val?1.f:0.f;
            if(GlobalSettings::instance()->model()->heightGrid()->valueAtIndex(x*2,y*2).isValid())
                mExternalSeedBaseMap->valueAtIndex(x,y) = -1.f;
        }
    QString path = GlobalSettings::instance()->path(GlobalSettings::instance()->settings().value("model.settings.seedDispersal.dumpSeedMapsPath"));

    if (GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.dumpSeedMapsEnabled",false)) {
#ifdef ILAND_GUI
        QImage img = gridToImage(*mExternalSeedBaseMap, true, -1., 2.);
        img.save(path + "/seedbeltmap_before.png");
#endif
    }
    //    img.save("seedmap.png");
    // now scan the pixels of the belt: paint all pixels that are close to the project area
    // we do this 4 times (for all cardinal direcitons)
    for (int y=0;y<mExternalSeedBaseMap->sizeY();y++) {
        for (int x=0;x<mExternalSeedBaseMap->sizeX();x++) {
            if (mExternalSeedBaseMap->valueAtIndex(x, y)!=1.)
                continue;
            int look_forward = std::min(x + seedbelt_width, mExternalSeedBaseMap->sizeX()-1);
            if (mExternalSeedBaseMap->valueAtIndex(look_forward, y)==-1.f) {
                // fill pixels
                for(; x<look_forward;++x) {
                    float &v = mExternalSeedBaseMap->valueAtIndex(x, y);
                    if (v==1.f) v=2.f;
                }
            }
        }
    }
    // right to left
    for (int y=0;y<mExternalSeedBaseMap->sizeY();y++) {
        for (int x=mExternalSeedBaseMap->sizeX();x>=0;--x) {
            if (mExternalSeedBaseMap->valueAtIndex(x, y)!=1.)
                continue;
            int look_forward = std::max(x - seedbelt_width, 0);
            if (mExternalSeedBaseMap->valueAtIndex(look_forward, y)==-1.f) {
                // fill pixels
                for(; x>look_forward;--x) {
                    float &v = mExternalSeedBaseMap->valueAtIndex(x, y);
                    if (v==1.f) v=2.f;
                }
            }
        }
    }
    // up and down ***
    // from top to bottom
    for (int x=0;x<mExternalSeedBaseMap->sizeX();x++) {
        for (int y=0;y<mExternalSeedBaseMap->sizeY();y++) {

            if (mExternalSeedBaseMap->valueAtIndex(x, y)!=1.)
                continue;
            int look_forward = std::min(y + seedbelt_width, mExternalSeedBaseMap->sizeY()-1);
            if (mExternalSeedBaseMap->valueAtIndex(x, look_forward)==-1.) {
                // fill pixels
                for(; y<look_forward;++y) {
                    float &v = mExternalSeedBaseMap->valueAtIndex(x, y);
                    if (v==1.f) v=2.f;
                }
            }
        }
    }
    // bottom to top ***
    for (int y=0;y<mExternalSeedBaseMap->sizeY();y++) {
        for (int x=mExternalSeedBaseMap->sizeX();x>=0;--x) {
            if (mExternalSeedBaseMap->valueAtIndex(x, y)!=1.)
                continue;
            int look_forward = std::max(y - seedbelt_width, 0);
            if (mExternalSeedBaseMap->valueAtIndex(x, look_forward)==-1.) {
                // fill pixels
                for(; y>look_forward;--y) {
                    float &v = mExternalSeedBaseMap->valueAtIndex(x, y);
                    if (v==1.f) v=2.f;
                }
            }
        }
    }

    if (GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.dumpSeedMapsEnabled",false)) {
#ifdef ILAND_GUI
        QImage img = gridToImage(*mExternalSeedBaseMap, true, -1., 2.);
        img.save(path + "/seedbeltmap_after.png");
#endif
    }
    mExtSeedData.clear();
    int sectors_x = xml.valueInt("sizeX",0);
    int sectors_y = xml.valueInt("sizeY",0);
    if(sectors_x<1 || sectors_y<1)
        throw IException(QString("setup of external seed dispersal: invalid number of sectors x=%1 y=%3").arg(sectors_x).arg(sectors_y));
    QDomElement elem = xml.node(".");
    for(QDomNode n = elem.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.nodeName().startsWith("species")) {
            QStringList coords = n.nodeName().split("_");
            if (coords.count()!=3)
                throw IException("external seed species definition is not valid: " + n.nodeName());
            int x = coords[1].toInt();
            int y = coords[2].toInt();
            if (x<0 || x>=sectors_x || y<0 || y>=sectors_y)
                throw IException(QString("invalid sector for specifiing external seed input (x y): %1 %2 ").arg(x).arg(y) );
            int index = y*sectors_x + x;

            QString text = xml.value("." + n.nodeName());
            qDebug() << "processing element " << n.nodeName() << "x,y:" << x << y << text;
            // we assume pairs of name and fraction
            QStringList species_list = text.split(" ");
            for (int i=0;i<species_list.count();++i) {
                QVector<double> &space = mExtSeedData[species_list[i]];
                if (space.isEmpty())
                    space.resize(sectors_x*sectors_y); // are initialized to 0s
                double fraction = species_list[++i].toDouble();
                space[index] = fraction;
            }
        }
    }
    mExtSeedSizeX = sectors_x;
    mExtSeedSizeY = sectors_y;
    qDebug() << "setting up of external seed maps finished";
}

void SeedDispersal::finalizeExternalSeeds()
{
    if (mExternalSeedBaseMap)
        delete mExternalSeedBaseMap;
    mExternalSeedBaseMap = 0;
}

void SeedDispersal::seedProductionSerotiny(const QPoint &position_index)
{
    if (mSeedMapSerotiny.isEmpty())
        throw IException("Invalid use seedProductionSerotiny(): tried to set a seed source for a non-serotinous species!");
    mSeedMapSerotiny.valueAtIndex(position_index.x()/mIndexFactor, position_index.y()/mIndexFactor)=1.f;
    mHasPendingSerotiny = true;
}

// ************ Kernel **************
void SeedDispersal::createKernel(Grid<float> &kernel, const double max_seed, const double scale_area)
{

    double max_dist = treemig_distanceTo(mKernelThresholdArea / species()->fecundity_m2());
    double cell_size = mSeedMap.cellsize();
    int max_radius = int(max_dist / cell_size);
    // e.g.: cell_size: regeneration grid (e.g. 400qm), px-size: light-grid (4qm)
    double occupation = cell_size*cell_size / (cPxSize*cPxSize * mTM_occupancy);

    kernel.clear();

    kernel.setup(mSeedMap.cellsize(), 2*max_radius + 1 , 2*max_radius + 1);
    int kernel_offset = max_radius;

    // filling of the kernel.... use the treemig density function
    double dist_center_cell = sqrt(cell_size*cell_size/M_PI);
    QPoint center = QPoint(kernel_offset, kernel_offset);
    const float *sk_end = kernel.end();
    for (float *p=kernel.begin(); p!=sk_end;++p) {
        double d = kernel.distance(center, kernel.indexOf(p));
        if (d==0.)
            *p = treemig_centercell(dist_center_cell); // r is the radius of a circle with the same area as a cell
        else
            *p = d<=max_dist?static_cast<float>(( treemig(d+dist_center_cell) + treemig(d-dist_center_cell))/2.f * cell_size*cell_size ):0.f;
    }

    // normalize
    float sum = kernel.sum();
    if (sum==0. || occupation==0.)
        throw IException("create seed kernel: sum of probabilities = 0!");

    // the sum of all kernel cells has to equal 1 (- long distance dispersal)
     kernel.multiply(scale_area/sum);


    if (mProbMode) {
        // probabilities are derived in multiplying by seed number, and dividing by occupancy criterion
        float fecundity_factor = static_cast<float>( max_seed / occupation);
        kernel.multiply( fecundity_factor );
        // all cells that get more seeds than the occupancy criterion are considered to have no seed limitation for regeneration
        for (float *p=kernel.begin(); p!=sk_end;++p) {
            *p = qMin(*p, 1.f);
        }
    }
    // set the parent cell to 1
    //kernel.valueAtIndex(kernel_offset, kernel_offset)=1.f;


    // some final statistics....
    if (logLevelInfo())
        qDebug() << "kernel setup. Species:"<< mSpecies->id() << "kernel-size: " << kernel.sizeX() << "x" << kernel.sizeY() << "pixels, sum (after scaling): " << kernel.sum();


}

double SeedDispersal::setupLDD()
{
    mLDDDensity.clear(); mLDDDistance.clear();
    if (mKernelThresholdLDD >= mKernelThresholdArea) {
        // no long distance dispersal
        return 0.;

    }
    double r_min = treemig_distanceTo(mKernelThresholdArea / species()->fecundity_m2());
    double r_max = treemig_distanceTo(mKernelThresholdLDD / species()->fecundity_m2());


    mLDDDistance.push_back(r_min);
    double ldd_sum = 0.;
    for (int i=0;i<mLDDRings;++i) {
        double r_in = mLDDDistance.last();
        mLDDDistance.push_back(mLDDDistance.last() + (r_max-r_min)/static_cast<float>(mLDDRings));
        double r_out = mLDDDistance.last();
        // calculate the value of the kernel for the middle of the ring
        double ring_in = treemig(r_in); // kernel value at the inner border of the ring
        double ring_out = treemig(r_out); // kernel value at the outer border of the ring
        double ring_val = ring_in*0.4 + ring_out*0.6; // this is the average p -- 0.4/0.6 better estimate the nonlinear behavior (fits very well for medium to large kernels, e.g. piab)
        //
        // calculate the area of the ring
        double ring_area = (r_out*r_out - r_in*r_in)*M_PI; // in square meters
        // the number of px considers the fecundity
        double n_px = ring_val * ring_area * species()->fecundity_m2() / mLDDSeedlings;
        ldd_sum += ring_val * ring_area; // this fraction of the full kernel (=1) is distributed in theis ring

        mLDDDensity.push_back(n_px);
    }
    qDebug() << "Setup LDD for" << species()->name() << ", using probability: "<< mLDDSeedlings<< ": Distances:" << mLDDDistance << ", seed pixels:" << mLDDDensity << "covered prob:" << ldd_sum;

    return ldd_sum;
}

/* R-Code:
treemig=function(as1,as2,ks,d) # two-part exponential function, cf. Lischke & Loeffler (2006), Annex
        {
        p1=(1-ks)*exp(-d/as1)/as1
        if(as2>0){p2=ks*exp(-d/as2)/as2}else{p2=0}
        p1+p2
        }
*/

/// the used kernel function
/// see also Appendix B of iland paper II (note the different variable names)
/// the function returns the seed density at a point with distance 'distance'.
double SeedDispersal::treemig(const double &distance)
{
    double p1 = (1.-mTM_ks)*exp(-distance/mTM_as1)/mTM_as1;
    double p2 = 0.;
    if (mTM_as2>0.)
       p2 = mTM_ks*exp(-distance/mTM_as2)/mTM_as2;
    double s = p1 + p2;
    // 's' is the density for radius 'distance' - not for specific point with that distance.
    // (i.e. the integral over the one-dimensional treemig function is 1, but if applied for 2d cells, the
    // sum would be much larger as all seeds arriving at 'distance' would be arriving somewhere at the circle with radius 'distance')
    // convert that to a density at a point, by dividing with the circumference at the circle with radius 'distance'
    s = s / (2.*std::max(distance, 0.01)*M_PI);

    return s;
}

double SeedDispersal::treemig_centercell(const double &max_distance)
{
    // use 100 steps and calculate dispersal kernel for consecutive rings
    double sum = 0.;
    for (int i=0;i<100;i++) {
        double r_in = i*max_distance/100.;
        double r_out = (i+1)*max_distance/100.;
        double ring_area = (r_out*r_out-r_in*r_in)*M_PI;
        // the value of each ring is: treemig(r) * area of the ring
        sum += treemig((r_out+r_in)/2.)*ring_area;
    }
    return sum;
}

/// calculate the distance where the probability falls below 'value'
double SeedDispersal::treemig_distanceTo(const double value)
{
    double dist = 0.;
    while (treemig(dist)>value && dist<10000.)
        dist+=10;
    return dist;
}

void SeedDispersal::setupExternalSeedsForSpecies(Species *species)
{
    if (!mExtSeedData.contains(species->id()))
        return; // nothing to do
    qDebug() << "setting up external seed map for" << species->id();
    QVector<double> &pcts = mExtSeedData[species->id()];
    mExternalSeedMap.setup(mSeedMap);
    mExternalSeedMap.initialize(0.f);
    for (int sector_x=0; sector_x<mExtSeedSizeX; ++sector_x)
        for (int sector_y=0; sector_y<mExtSeedSizeY; ++sector_y) {
            int xmin,xmax,ymin,ymax;
            int fx = mExternalSeedMap.sizeX() / mExtSeedSizeX; // number of cells per sector
            xmin = sector_x*fx;
            xmax = (sector_x+1)*fx;
            fx = mExternalSeedMap.sizeY() / mExtSeedSizeY; // number of cells per sector
            ymin = sector_y*fx;
            ymax = (sector_y+1)*fx;
            // now loop over the whole sector
            int index = sector_y*mExtSeedSizeX  + sector_x;
            double p = pcts[index];
            for (int y=ymin;y<ymax;++y)
                for (int x=xmin;x<xmax;++x) {
                    // check
                    if (mExternalSeedBaseMap->valueAtIndex(x,y)==2.f)
                        if (drandom()<p)
                            mExternalSeedMap.valueAtIndex(x,y) = 1.f; // flag
                }

        }
    if (!mProbMode) {
       // scale external seed values to have pixels with LAI=3
        for (float *p=mExternalSeedMap.begin(); p!=mExternalSeedMap.end(); ++p)
           *p *= 3.f * mExternalSeedMap.cellsize()*mExternalSeedMap.cellsize();
    }
}


// ************ Dispersal **************


/// debug function: loads a image of arbirtrary size...
void SeedDispersal::loadFromImage(const QString &fileName)
{
    mSeedMap.clear();
    loadGridFromImage(fileName, mSeedMap);
    for (float* p=mSeedMap.begin();p!=mSeedMap.end();++p)
        *p = *p>0.8?1.f:0.f;

}

void SeedDispersal::clear()
{
    Grid<float> *seed_map = &mSeedMap;
    if (!mProbMode) {
        seed_map = &mSourceMap;
        mSeedMap.initialize(0.f);
    }
    if (!mExternalSeedMap.isEmpty()) {
        // we have a preprocessed initial value for the external seed map (see setupExternalSeeds() et al)
        seed_map->copy(mExternalSeedMap);
        return;
    }
    // clear the map
    float background_value = static_cast<float>(mExternalSeedBackgroundInput); // there is potentitally a background probability <>0 for all pixels.
    seed_map->initialize(background_value);
    if (mHasExternalSeedInput) {
        // if external seed input is enabled, the buffer area of the seed maps is
        // "turned on", i.e. set to 1.
        int buf_size = GlobalSettings::instance()->settings().valueInt("model.world.buffer",0.) / static_cast<int>(seed_map->cellsize());
        // if a special buffer is defined, reduce the size of the input
        if (mExternalSeedBuffer>0)
            buf_size -= mExternalSeedBuffer;
        if (buf_size>0) {
            int ix,iy;
            for (iy=0;iy<seed_map->sizeY();++iy)
                for (ix=0;ix<seed_map->sizeX(); ++ix)
                    if (iy<buf_size || iy>=seed_map->sizeY()-buf_size || ix<buf_size || ix>=seed_map->sizeX()-buf_size) {
                        if (mExternalSeedDirection==0) {
                            // seeds from all directions
                            seed_map->valueAtIndex(ix,iy)=1.f;
                        } else {
                            // seeds only from specific directions
                            float value = 0.f;
                            if (isBitSet(mExternalSeedDirection,1) && ix>=seed_map->sizeX()-buf_size) value = 1; // north
                            if (isBitSet(mExternalSeedDirection,2) && iy<buf_size) value = 1; // east
                            if (isBitSet(mExternalSeedDirection,3) && ix<buf_size) value = 1; // south
                            if (isBitSet(mExternalSeedDirection,4) && iy>=seed_map->sizeY()-buf_size) value = 1; // west
                            seed_map->valueAtIndex(ix,iy)=value;
                        }
                    }
        } else {
            qDebug() << "external seed input: Error: invalid buffer size???";
        }
    }
}

static int _debug_ldd=0;
void SeedDispersal::execute()
{
#ifdef ILAND_GUI
    int year = GlobalSettings::instance()->currentYear();
    QString path;
    if (mDumpSeedMaps) {
        path = GlobalSettings::instance()->path( GlobalSettings::instance()->settings().value("model.settings.seedDispersal.dumpSeedMapsPath") );
        gridToImage(seedMap(), true, 0., 1.).save(QString("%1/seed_before_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
        qDebug() << "saved seed map image to" << path;
    }
#else
    if (mDumpSeedMaps)
        qDebug() << "saving of seedmaps only supported in the iLand GUI.";
#endif
    if (mProbMode) {

        DebugTimer t("seed dispersal", true);

        // (1) detect edges
        if (edgeDetection()) {

#ifdef ILAND_GUI
            if (mDumpSeedMaps) {
                gridToImage(seedMap(), true, -1., 1.).save(QString("%1/seed_edge_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
            }
#endif

            // (2) distribute seed probabilites from edges
            distribute();
        }

        // special case serotiny
        if (mHasPendingSerotiny) {
            qDebug() << "calculating extra seed rain (serotiny)....";
#ifdef ILAND_GUI
            if (mDumpSeedMaps) {
                gridToImage(mSeedMapSerotiny, true, 0., 1.).save(QString("%1/seed_serotiny_before_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
            }
#endif
            if (edgeDetection(&mSeedMapSerotiny))
                distribute(&mSeedMapSerotiny);
            // copy back data
            float *sero=mSeedMapSerotiny.begin();
            for (float* p=mSeedMap.begin();p!=mSeedMap.end();++p, ++sero)
                *p = std::max(*p, *sero);

            float total = mSeedMapSerotiny.sum();
#ifdef ILAND_GUI
            if (mDumpSeedMaps) {
                gridToImage(mSeedMapSerotiny, true, 0., 1.).save(QString("%1/seed_serotiny_after_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
            }
#endif
            mSeedMapSerotiny.initialize(0.f); // clear
            mHasPendingSerotiny = false;
            qDebug() << "serotiny event: extra seed input" << total << "(total sum of seed probability over all pixels of the serotiny seed map) of species" << mSpecies->name();
        }

    } else {
        // distribute actual values
        DebugTimer t("seed dispersal", true);
        // fill seed map from source map
        distributeSeeds();

    }
#ifdef ILAND_GUI
    if (mDumpSeedMaps) {
        //qDebug() << "finished seed dispersal for species. time: " << mSpecies->id() << t.elapsed();
        gridToImage(seedMap(), true, 0., 1.).save(QString("%1/seed_after_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
    }

    if (!mDumpNextYearFileName.isEmpty()) {
        Helper::saveToTextFile(GlobalSettings::instance()->path(mDumpNextYearFileName), gridToESRIRaster(seedMap()));
        qDebug() << "saved seed map for " << species()->id() << "to" << GlobalSettings::instance()->path(mDumpNextYearFileName);
        mDumpNextYearFileName = QString();
    }
    qDebug() << "LDD-count:" << _debug_ldd;

#endif
}

/** scans the seed image and detects "edges".
    edges are then subsequently marked (set to -1). This is pass 1 of the seed distribution process.
*/
bool SeedDispersal::edgeDetection(Grid<float> *seed_map)
{
    float *p_above, *p, *p_below;
    Grid<float> &seedmap = seed_map ? *seed_map : mSeedMap; // switch to extra seed map if provided
    int dy = seedmap.sizeY();
    int dx = seedmap.sizeX();
    int x,y;
    bool found = false;

    // fill mini-gaps
    int n_gaps_filled=0;
    for (y=1;y<dy-1;++y){
        p = seedmap.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            if (*p < 0.999f) {

                if ((*(p_above-1)==1.f) + (*p_above==1.f) + (*(p_above+1)==1.f) +
                    (*(p-1)==1.f) + (*(p+1)==1.f) +
                    (*(p_below-1)==1.f) + (*p_below==1.f) + (*(p_below+1)==1.f) > 3) {
                    *p=0.999f; // if more than 3 neighbors are active pixels, the value is high
                    ++n_gaps_filled;
                }
            }

        }
    }


    // now detect the edges
    int n_edges=0 ;
    for (y=1;y<dy-1;++y){
        p = seedmap.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            if (*p == 1.f) {
                found = true;
                if ( (*(p_above-1)<0.999f && *(p_above-1)>=0.f)
                     || (*p_above<0.999f && *p_above>=0.f)
                     || (*(p_above+1)<0.999f && *(p_above+1)>=0.f)
                     || (*(p-1)<0.999f && *(p-1)>=0.f)
                     || (*(p+1)<0.999f && (*p+1)>=0.f)
                     || (*(p_below-1)<0.999f && *(p_below-1)>=0.f)
                     || (*p_below<0.999f && *p_below>=0.f)
                     || (*(p_below+1)<0.999f && *(p_below+1)>=0.f ) ) {
                    *p=-1.f; // if any surrounding pixel is >=0 & <0.999: -> mark as edge
                    ++n_edges;
                }
            }

        }
    }
    if (mDumpSeedMaps)
        qDebug() << "species:" << mSpecies->id() << "# of gaps filled: " << n_gaps_filled << "# of edge-pixels:" << n_edges;
    return found;
}

/** do the seed probability distribution.
    This is phase 2. Apply the seed kernel for each "edge" point identified in phase 1.
*/
void SeedDispersal::distribute(Grid<float> *seed_map)
{
    int x,y;
    Grid<float> &seedmap = seed_map ? *seed_map : mSeedMap; // switch to extra seed map if provided
    float *end = seedmap.end();
    float *p = seedmap.begin();
    // choose the kernel depending whether there is a seed year for the current species or not
    Grid<float> *kernel = species()->isSeedYear()? &mKernelSeedYear : &mKernelNonSeedYear;
    // extra case: serotiny
    if (seed_map)
        kernel = &mKernelSerotiny;

    int offset = kernel->sizeX() / 2; // offset is the index of the center pixel
    for(;p!=end;++p) {
        if (*p==-1.f) {
            // edge pixel found. Now apply the kernel....
            QPoint pt=seedmap.indexOf(p);
            for (y=-offset;y<=offset;++y) {
                for (x=-offset;x<=offset;++x) {
                    float &kernel_value = kernel->valueAtIndex(x+offset, y+offset);
                    if (kernel_value>0.f && seedmap.isIndexValid(pt.x()+x, pt.y()+y)) {
                        float &val = seedmap.valueAtIndex(pt.x()+x, pt.y()+y);
                        if (val!=-1.f)
                            val = qMin(1.f - (1.f - val)*(1.f-kernel_value),1.f );
                    }
                }
            }
            // long distance dispersal
            if (!mLDDDensity.isEmpty()) {
                double m = species()->isSeedYear() ? 1. : mNonSeedYearFraction;
                for (int r=0;r<mLDDDensity.size(); ++r) {
                    float ldd_val = mLDDSeedlings; // pixels will have this probability
                    int n = round( mLDDDensity[r]*m ); // number of pixels to activate
                    for (int i=0;i<n;++i) {
                        // distance and direction:
                        double radius = nrandom(mLDDDistance[r], mLDDDistance[r+1]) / seedmap.cellsize(); // choose a random distance (in pixels)
                        double phi = drandom()*2.*M_PI; // choose a random direction
                        QPoint ldd(pt.x() + radius*cos(phi), pt.y() + radius*sin(phi));
                        if (seedmap.isIndexValid(ldd)) {
                            float &val = seedmap.valueAtIndex(ldd);
                            _debug_ldd++;
                            // use the same adding of probabilities
                            if (val!=-1.f)
                                val = qMin(1.f - (1.f - val)*(1.f-ldd_val), 1.f);
                        }
                    }
                }
            }
            *p=1.f; // mark as processed
        } // *p==1
    } // for()
}

void SeedDispersal::distributeSeeds(Grid<float> *seed_map)
{
    Grid<float> &sourcemap = seed_map ? *seed_map : mSourceMap; // switch to extra seed map if provided
    Grid<float> &kernel = mKernelSeedYear;
    // calculate number of seeds; the source map holds now m2 leaf area on 20x20m pixels
    // after this step, each source cell has a value between 0 (no source) and 1 (fully covered cell)
    float fec = species()->fecundity_m2();
    if (!species()->isSeedYear())
        fec *= mNonSeedYearFraction;
    for (float *p=sourcemap.begin(); p!=sourcemap.end(); ++p){
        if (*p) {
            // if LAI  >3, then full potential is assumed, below LAI=3 a linear ramp is used
            *p = std::min(*p / (sourcemap.cellsize()*sourcemap.cellsize()) /3.f, 3.f);
        }
    }

    // sink mode

//    // now look for each pixel in the targetmap and sum up seeds*kernel
//    int idx=0;
//    int offset = kernel.sizeX() / 2; // offset is the index of the center pixel
//    //const Grid<ResourceUnit*> &ru_map = GlobalSettings::instance()->model()->RUgrid();
//    DebugTimer tsink("seed_sink"); {
//    for (float *t=mSeedMap.begin(); t!=mSeedMap.end(); ++t, ++idx) {
//        // skip out-of-project areas
//        //if (!ru_map.constValueAtIndex(mSeedMap.index5(idx)))
//        //    continue;
//        // apply the kernel
//        QPoint sm=mSeedMap.indexOf(t)-QPoint(offset, offset);
//        for (int iy=0;iy<kernel.sizeY();++iy) {
//            for (int ix=0;ix<kernel.sizeX();++ix) {
//                if (sourcemap.isIndexValid(sm.x()+ix, sm.y()+iy))
//                    *t+=sourcemap(sm.x()+ix, sm.y()+iy) * kernel(ix, iy);
//            }
//        }
//    }
//    } // debugtimer
//    mSeedMap.initialize(0.f); // just for debugging...

    int offset = kernel.sizeX() / 2; // offset is the index of the center pixel
    // source mode


    for (float *src=sourcemap.begin(); src!=sourcemap.end(); ++src) {
        if (*src>0.f) {
            QPoint sm=sourcemap.indexOf(src)-QPoint(offset, offset);
            int sx = sm.x(), sy=sm.y();
            for (int iy=0;iy<kernel.sizeY();++iy) {
                for (int ix=0;ix<kernel.sizeX();++ix) {
                    if (mSeedMap.isIndexValid(sx+ix, sy+iy))
                        mSeedMap.valueAtIndex(sx+ix, sy+iy)+= *src * kernel(ix, iy);
                }
            }
            // long distance dispersal
            if (!mLDDDensity.isEmpty()) {
                QPoint pt=sourcemap.indexOf(src);

                for (int r=0;r<mLDDDensity.size(); ++r) {
                    float ldd_val = mLDDSeedlings / fec; // pixels will have this probability [note: fecundity will be multiplied below]
                    int n;
                    if (mLDDDensity[r]<1)
                        n = drandom()<mLDDDensity[r] ? 1 : 0;
                    else
                        n = round( mLDDDensity[r] ); // number of pixels to activate
                    for (int i=0;i<n;++i) {
                        // distance and direction:
                        double radius = nrandom(mLDDDistance[r], mLDDDistance[r+1]) / mSeedMap.cellsize(); // choose a random distance (in pixels)
                        double phi = drandom()*2.*M_PI; // choose a random direction
                        QPoint ldd(pt.x() + radius*cos(phi), pt.y() + radius*sin(phi));
                        if (mSeedMap.isIndexValid(ldd)) {
                            float &val = mSeedMap.valueAtIndex(ldd);
                            _debug_ldd++;
                            val += ldd_val;
                        }
                    }
                }
            }

        }
    }



    // now the seed sources (0..1) are spatially distributed by the kernel (and LDD) without altering the magnitude;
    // now we include the fecundity (=seedling potential per m2 crown area), and convert to the establishment probability p_seed.
    // The number of (potential) seedlings per m2 on each cell is: cell * fecundity[m2]
    // We assume that the availability of 10 potential seedlings/m2 is enough for unconstrained establishment;
    const float n_unlimited = 100.f;
    for (float *p=mSeedMap.begin(); p!=mSeedMap.end(); ++p){
        if (*p>0.f) {
            *p = std::min(*p*fec / n_unlimited, 1.f);
        }
    }
}
