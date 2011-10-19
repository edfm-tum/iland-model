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
#include "helper.h"
#include "species.h"

#include <QtGui/QImage>

/** @class SeedDispersal
    The class encapsulates the dispersal of seeds of one species over the whole landscape.

  */
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

    const float seedmap_size = 20.f;
    // setup of seed map
    mSeedMap.clear();
    mSeedMap.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), seedmap_size );
    mSeedMap.initialize(0.);
    mIndexFactor = int(seedmap_size) / cPxSize; // ratio seed grid / lip-grid:
    if (logLevelInfo()) qDebug() << "Seed map setup. Species:"<< mSpecies->id() << "kernel-size: " << mSeedMap.sizeX() << "x" << mSeedMap.sizeY() << "pixels.";

    if (mSpecies==0)
        throw IException("Setup of SeedDispersal: Species not defined.");

    // settings
    mTM_occupancy = 1.; // is currently constant
    mSpecies->treeMigKernel(mTM_as1, mTM_as2, mTM_ks); // set values
    mTM_fecundity_cell = mSpecies->fecundity_m2() * seedmap_size*seedmap_size * mTM_occupancy; // scale to production for the whole cell
    mNonSeedYearFraction = mSpecies->nonSeedYearFraction();

    createKernel(mKernelSeedYear, mTM_fecundity_cell);

    // the kernel for non seed years looks similar, but is simply linearly scaled down
    // using the species parameter NonSeedYearFraction.
    // the central pixel still gets the value of 1 (i.e. 100% probability)
    createKernel(mKernelNonSeedYear, mTM_fecundity_cell*mNonSeedYearFraction);

    // debug info
    mDumpSeedMaps = GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.dumpSeedMapsEnabled",false);
    if (mDumpSeedMaps) {
        QString path = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.dumpSeedMapsPath");
        Helper::saveToTextFile(QString("%1/seedkernelYes_%2.csv").arg(path).arg(mSpecies->id()),gridToString(mKernelSeedYear));
        Helper::saveToTextFile(QString("%1/seedkernelNo_%2.csv").arg(path).arg(mSpecies->id()),gridToString(mKernelNonSeedYear));
    }
    // external seeds
    mHasExternalSeedInput = false;
    mExternalSeedBuffer = 0;
    mExternalSeedDirection = 0;
    if (GlobalSettings::instance()->settings().valueBool("model.settings.seedDispersal.externalSeedEnabled",false)) {
        // current species in list??
        mHasExternalSeedInput = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedSpecies").contains(mSpecies->id());
        QString dir = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedSource").toLower();
        // encode cardinal positions as bits: e.g: "e,w" -> 6
        mExternalSeedDirection += dir.contains("n")?1:0;
        mExternalSeedDirection += dir.contains("e")?2:0;
        mExternalSeedDirection += dir.contains("s")?4:0;
        mExternalSeedDirection += dir.contains("w")?8:0;
        QStringList buffer_list = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.externalSeedBuffer").simplified().split(',');
        int index = buffer_list.indexOf(mSpecies->id());
        if (index>=0) {
            mExternalSeedBuffer = buffer_list[index+1].toInt();
            qDebug() << "enabled special buffer for species" <<mSpecies->id() << ": distance of" << mExternalSeedBuffer << "pixels = " << mExternalSeedBuffer*20. << "m";
        }
        if (mHasExternalSeedInput)
            qDebug() << "External seed input enabled for" << mSpecies->id();
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

// ************ Kernel **************
void SeedDispersal::createKernel(Grid<float> &kernel, const float max_seed)
{

    double max_dist = treemig_distanceTo(0.0001);
    double cell_size = mSeedMap.cellsize();
    int max_radius = int(max_dist / cell_size);
    // e.g.: cell_size: regeneration grid (e.g. 400qm), px-size: light-grid (4qm)
    double occupation = cell_size*cell_size / (cPxSize*cPxSize * mTM_occupancy);

    kernel.clear();

    kernel.setup(mSeedMap.cellsize(), 2*max_radius + 1 , 2*max_radius + 1);
    int kernel_offset = max_radius;

    // filling of the kernel.... use the treemig
    QPoint center = QPoint(kernel_offset, kernel_offset);
    const float *sk_end = kernel.end();
    for (float *p=kernel.begin(); p!=sk_end;++p) {
        double d = kernel.distance(center, kernel.indexOf(p));
        *p = d<=max_dist?treemig(d):0.f;
    }

    // normalize
    double sum = kernel.sum();
    if (sum==0. || occupation==0.)
        throw IException("create seed kernel: sum of probabilities = 0!");

    // the sum of all kernel cells has to equal 1
    kernel.multiply(1./sum);
    // probabilities are derived in multiplying by seed number, and dividing by occupancy criterion
    kernel.multiply( max_seed / occupation);
    // all cells that get more seeds than the occupancy criterion are considered to have no seed limitation for regeneration
    for (float *p=kernel.begin(); p!=sk_end;++p) {
        *p = qMin(*p, 1.f);
    }
    // set the parent cell to 1
    kernel.valueAtIndex(kernel_offset, kernel_offset)=1.f;


    // some final statistics....
    if (logLevelInfo()) qDebug() << "kernel setup.Species:"<< mSpecies->id() << "kernel-size: " << kernel.sizeX() << "x" << kernel.sizeY() << "pixels, sum: " << kernel.sum();
}

/* R-Code:
treemig=function(as1,as2,ks,d) # two-part exponential function, cf. Lischke & Löffler (2006), Annex
        {
        p1=(1-ks)*exp(-d/as1)/as1
        if(as2>0){p2=ks*exp(-d/as2)/as2}else{p2=0}
        p1+p2
        }
*/

/// the used kernel function
/// see also Appendix B of iland paper II (note the different variable names)
double SeedDispersal::treemig(const double &distance)
{
    double p1 = (1.-mTM_ks)*exp(-distance/mTM_as1)/mTM_as1;
    double p2 = 0.;
    if (mTM_as2>0.)
       p2 = mTM_ks*exp(-distance/mTM_as2)/mTM_as2;
    return p1 + p2;
}

/// calculate the distance where the probability falls below 'value'
double SeedDispersal::treemig_distanceTo(const double value)
{
    double dist = 0.;
    while (treemig(dist)>value && dist<10000.)
        dist+=10;
    return dist;
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
    mSeedMap.initialize(0.f);
    if (mHasExternalSeedInput) {
        // if external seed input is enabled, the buffer area of the seed maps is
        // "turned on", i.e. set to 1.
        int buf_size = GlobalSettings::instance()->settings().valueDouble("model.world.buffer",0.) / mSeedMap.cellsize();
        // if a special buffer is defined, reduce the size of the input
        if (mExternalSeedBuffer>0)
            buf_size -= mExternalSeedBuffer;
        if (buf_size>0) {
            int ix,iy;
            for (iy=0;iy<mSeedMap.sizeY();++iy)
                for (ix=0;ix<mSeedMap.sizeX(); ++ix)
                    if (iy<buf_size || iy>=mSeedMap.sizeY()-buf_size || ix<buf_size || ix>=mSeedMap.sizeX()-buf_size) {
                        if (mExternalSeedDirection==0) {
                            // seeds from all directions
                            mSeedMap.valueAtIndex(ix,iy)=1.f;
                        } else {
                            // seeds only from specific directions
                            float value = 0.f;
                            if (isBitSet(mExternalSeedDirection,1) && ix>=mSeedMap.sizeX()-buf_size) value = 1; // north
                            if (isBitSet(mExternalSeedDirection,2) && iy<buf_size) value = 1; // east
                            if (isBitSet(mExternalSeedDirection,3) && ix<buf_size) value = 1; // south
                            if (isBitSet(mExternalSeedDirection,4) && iy>=mSeedMap.sizeY()-buf_size) value = 1; // west
                            mSeedMap.valueAtIndex(ix,iy)=value;
                        }
                    }
        } else {
            qDebug() << "external seed input: Error: invalid buffer size???";
        }
    }
}

void SeedDispersal::execute()
{
    int year = GlobalSettings::instance()->currentYear();
    QString path;
    if (mDumpSeedMaps) {
        path = GlobalSettings::instance()->settings().value("model.settings.seedDispersal.dumpSeedMapsPath");
        gridToImage(seedMap(), true, 0., 1.).save(QString("%1/seed_before_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
        qDebug() << "saved seed map image to" << path;
    }
    {
    DebugTimer t("seed dispersal");
    // (1) detect edges
    if (edgeDetection()) {
    // (2) distribute seed probabilites from edges
        distribute();
    }
    }
    if (mDumpSeedMaps)
        gridToImage(seedMap(), true, 0., 1.).save(QString("%1/seed_after_%2_%3.png").arg(path).arg(mSpecies->id()).arg(year));
}

/** scans the seed image and detects "edges".
    edges are then subsequently marked (set to -1). This is pass 1 of the seed distribution process.
*/
bool SeedDispersal::edgeDetection()
{
    float *p_above, *p, *p_below;
    int dy = mSeedMap.sizeY();
    int dx = mSeedMap.sizeX();
    int x,y;
    bool found = false;
    for (y=1;y<dy-1;++y){
        p = mSeedMap.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            if (*p == 1.) {
                found = true;
                if (*(p_above-1)==0.f || *p_above==0.f || *(p_above+1)==0.f ||
                    *(p-1)==0.f || *(p+1)==0.f ||
                    *(p_below-1)==0.f || *p_below==0.f || *(p_below+1)==0.f )
                    *p=-1; // if any surrounding pixel is 0: -> mark as edge
            }

        }
    }
    return found;
}

/** do the seed probability distribution.
    This is phase 2. Apply the seed kernel for each "edge" point identified in phase 1.
*/
void SeedDispersal::distribute()
{
    int x,y;

    float *end = mSeedMap.end();
    float *p = mSeedMap.begin();
    // choose the kernel depending whether there is a seed year for the current species or not
    Grid<float> &kernel = species()->isSeedYear()?mKernelSeedYear:mKernelNonSeedYear;
    int offset = kernel.sizeX() / 2; // offset is the index of the center pixel
    for(;p!=end;++p) {
        if (*p==-1.f) {
            // edge pixel found. Now apply the kernel....
            QPoint pt=mSeedMap.indexOf(p);
            for (y=-offset;y<=offset;++y)
                for (x=-offset;x<=offset;++x)
                    if (mSeedMap.isIndexValid(pt.x()+x, pt.y()+y)) {
                        float &val = mSeedMap.valueAtIndex(pt.x()+x, pt.y()+y);
                        if (val!=-1)
                            val = qMin(1.f - (1.f - val)*(1.f-kernel.valueAtIndex(x+offset, y+offset)),1.f );
                    }
            *p=1.f; // mark as processed
        } // *p==1
    } // for()
}
