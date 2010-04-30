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
    mIndexOffset = GlobalSettings::instance()->model()->grid()->indexAt(QPointF(0., 0.));
    mIndexFactor = int(seedmap_size) / cPxSize; // ratio seed grid / lip-grid:
    qDebug() << "Seed map setup. Species:"<< mSpecies->id() << "kernel-size: " << mSeedMap.sizeX() << "x" << mSeedMap.sizeY() << "pixels, offset to (0/0): " << mIndexOffset;

    createKernel();
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

    //Helper::saveToTextFile("seedkernel.csv",gridToString(mSeedKernel));

    // randomize seed map.... set 1/3 to "filled"
    //for (int i=0;i<mSeedMap.count(); i++)
    //    mSeedMap.valueAtIndex(mSeedMap.randomPosition()) = 1.;


//    QImage img = gridToImage(mSeedMap, true, -1., 1.);
//    img.save("seedmap.png");

//    img = gridToImage(mSeedMap, true, -1., 1.);
//    img.save("seedmap_e.png");
}

// ************ Kernel **************
void SeedDispersal::createKernel()
{
    mTM_as1 = 100.;
    mTM_as2 = 0.;
    mTM_ks = 0.;
    mTM_maxseed = 10000.;
    mTM_required_seeds = 1.;
    double max_dist = treemig_distanceTo(0.0001);
    double cell_size = mSeedMap.cellsize();
    int max_radius = qRound(max_dist / cell_size);
    // e.g.: cell_size: regeneration grid (e.g. 400qm), px-size: light-grid (4qm)
    double occupation = cell_size*cell_size / (cPxSize*cPxSize * mTM_required_seeds); //cell.size.disp^2/(cell.size.regen^2*requ.seeds)

    mSeedKernel.clear();
    mSeedKernel.setup(mSeedMap.cellsize(), 2*max_radius + 1 , 2*max_radius + 1);
    mKernelOffset = max_radius;

    // filling of the kernel.... use the treemig
    QPoint center = QPoint(mKernelOffset, mKernelOffset);
    const float *sk_end = mSeedKernel.end();
    for (float *p=mSeedKernel.begin(); p!=sk_end;++p) {
        double d = mSeedKernel.distance(center, mSeedKernel.indexOf(p));
        *p = d<=max_dist?treemig(d):0.f;
    }

    // normalize
    double sum = mSeedKernel.sum();
    if (sum==0. || occupation==0.)
        throw IException("create seed kernel: sum of probabilities = 0!");

    // the sum of all kernel cells has to equal 1
    mSeedKernel.multiply(1./sum);
    // probabilities are derived in multiplying by seed number, and dividing by occupancy criterion
    mSeedKernel.multiply( mTM_maxseed / occupation);
    // all cells that get more seeds than the occupancy criterion are considered to have no seed limitation for regeneration
    for (float *p=mSeedKernel.begin(); p!=sk_end;++p) {
        *p = qMin(*p, 1.f);
    }
    // set the parent cell to 1
    mSeedKernel.valueAtIndex(mKernelOffset, mKernelOffset)=1.f;
    // some final statistics....
    qDebug() << "kernel setup.Species:"<< mSpecies->id() << "kernel-size: " << mSeedKernel.sizeX() << "x" << mSeedKernel.sizeY() << "pixels, sum: " << mSeedKernel.sum();
}

/* R-Code:
treemig=function(as1,as2,ks,d) # two-part exponential function, cf. Lischke & Löffler (2006), Annex
        {
        p1=(1-ks)*exp(-d/as1)/as1
        if(as2>0){p2=ks*exp(-d/as2)/as2}else{p2=0}
        p1+p2
        }
*/
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
}

void SeedDispersal::execute()
{
    static int img_counter = 0;
    gridToImage(seedMap(), true, 0., 1.).save(QString("e:\\temp\\seedmaps\\seed_before%1.png").arg(img_counter));
    {
    DebugTimer t("seed dispersal");
    // (1) detect edges
    edgeDetection();
    // (2) distribute seed probabilites from edges
    distribute();
    }
    gridToImage(seedMap(), true, 0., 1.).save(QString("e:\\temp\\seedmaps\\seed_after%1.png").arg(img_counter++));
}

/** scans the seed image and detects "edges".
    edges are then subsequently marked (set to -1). This is pass 1 of the seed distribution process.
*/
void SeedDispersal::edgeDetection()
{
    float *p_above, *p, *p_below;
    int dy = mSeedMap.sizeY();
    int dx = mSeedMap.sizeX();
    int x,y;
    for (y=1;y<dy-1;++y){
        p = mSeedMap.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            if (*p == 1.) {
                if (*(p_above-1)==0.f || *p_above==0.f || *(p_above+1)==0.f ||
                    *(p-1)==0.f || *(p+1)==0.f ||
                    *(p_below-1)==0.f || *p_below==0.f || *(p_below+1)==0.f )
                    *p=-1; // if any surrounding pixel is 0: -> mark as edge
            }

        }
    }
}

/** do the seed probability distribution.
    This is phase 2. Apply the seed kernel for each "edge" point identified in phase 1.
*/
void SeedDispersal::distribute()
{
    int x,y;

    float *end = mSeedMap.end();
    float *p = mSeedMap.begin();
    for(;p!=end;++p) {
        if (*p==-1.f) {
            // edge pixel found. Now apply the kernel....
            QPoint pt=mSeedMap.indexOf(p);
            for (y=-mKernelOffset;y<=mKernelOffset;++y)
                for (x=-mKernelOffset;x<=mKernelOffset;++x)
                    if (mSeedMap.isIndexValid(pt.x()+x, pt.y()+y)) {
                        float &val = mSeedMap.valueAtIndex(pt.x()+x, pt.y()+y);
                        if (val!=-1)
                            val = qMin(1.f - (1.f - val)*(1.f-mSeedKernel.valueAtIndex(x+mKernelOffset, y+mKernelOffset)),1.f );
                    }
            *p=1.f; // mark as processed
        } // *p==1
    } // for()
}
