#include "seeddispersal.h"

#include "globalsettings.h"
#include "model.h"
#include "helper.h"

#include <QtGui/QImage>

/** @class SeedDispersal
    The class encapsulates the dispersal of seeds of one species over the whole landscape.

  */
SeedDispersal::~SeedDispersal()
{
    if (isSetup()) {

    }
}

void SeedDispersal::setup()
{
    if (!GlobalSettings::instance()->model())
        return;
    // setup of seed map
    mSeedMap.clear();
    mSeedMap.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(),
                   GlobalSettings::instance()->model()->heightGrid()->cellsize() );
    mSeedMap.initialize(0.);

    // setup of seed kernel
    const int max_radius = 15; // pixels
    float cellsize = mSeedMap.cellsize();

    mSeedKernel.clear();
    mSeedKernel.setup(mSeedMap.cellsize(), 2*max_radius + 1 , 2*max_radius + 1);
    mOffset = max_radius;
    // filling of the kernel.... for simplicity: a linear kernel
    QPoint center = QPoint(mOffset, mOffset);
    const double max_dist = max_radius * cellsize;
    for (float *p=mSeedKernel.begin(); p!=mSeedKernel.end();++p) {
        double d = mSeedKernel.distance(center, mSeedKernel.indexOf(p));
        *p = qMax( 1. - d / max_dist, 0.);
    }

    //Helper::saveToTextFile("seedkernel.csv",gridToString(mSeedKernel));

    // randomize seed map.... set 1/3 to "filled"
    //for (int i=0;i<mSeedMap.count(); i++)
    //    mSeedMap.valueAtIndex(mSeedMap.randomPosition()) = 1.;


//    QImage img = gridToImage(mSeedMap, true, -1., 1.);
//    img.save("seedmap.png");

//    img = gridToImage(mSeedMap, true, -1., 1.);
//    img.save("seedmap_e.png");
}

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
    DebugTimer t("seed dispersal");
    // (1) detect edges
    edgeDetection();
    // (2) distribute seed probabilites from edges
    distribute();
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
            for (y=-mOffset;y<=mOffset;++y)
                for (x=-mOffset;x<=mOffset;++x)
                    if (mSeedMap.isIndexValid(pt.x()+x, pt.y()+y)) {
                        float &val = mSeedMap.valueAtIndex(pt.x()+x, pt.y()+y);
                        if (val!=-1)
                            val = qMin(1.f - (1.f - val)*(1.f-mSeedKernel.valueAtIndex(x+mOffset, y+mOffset)),1.f );
                    }
            *p=1.f; // mark as processed
        } // *p==1
    } // for()
}
