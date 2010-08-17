/** @class ResourceUnit
  ResourceUnit is the spatial unit that encapsulates a forest stand and links to several environmental components
  (Climate, Soil, Water, ...).

  */
#include <QtCore>
#include "global.h"

#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "speciesset.h"
#include "species.h"
#include "production3pg.h"
#include "model.h"
#include "climate.h"
#include "watercycle.h"
#include "helper.h"

ResourceUnit::~ResourceUnit()
{
    if (mWater)
        delete mWater;
}

ResourceUnit::ResourceUnit(const int index)
{
    mSpeciesSet = 0;
    mClimate = 0;
    mPixelCount=0;
    mIndex = index;
    mSaplingHeightMap = 0;
    mWater = new WaterCycle();

    mTrees.reserve(100); // start with space for 100 trees.
}

void ResourceUnit::setup()
{
    mWater->setup(this);
    // setup variables
    mUnitVariables.nitrogenAvailable = GlobalSettings::instance()->settings().valueDouble("model.site.availableNitrogen", 40);
    mAverageAging = 0.;

}
void ResourceUnit::setBoundingBox(const QRectF &bb)
{
    mBoundingBox = bb;
    mCornerCoord = GlobalSettings::instance()->model()->grid()->indexAt(bb.topLeft());
}

/// set species and setup the species-per-RU-data
void ResourceUnit::setSpeciesSet(SpeciesSet *set)
{
    mSpeciesSet = set;
    mRUSpecies.clear();
    mRUSpecies.resize(set->count()); // ensure that the vector space is not relocated
    for (int i=0;i<set->count();i++) {
        Species *s = const_cast<Species*>(mSpeciesSet->species(i));
        if (!s)
            throw IException("ResourceUnit::setSpeciesSet: invalid index!");

        /* be careful: setup() is called with a pointer somewhere to the content of the mRUSpecies container.
           If the container memory is relocated (QVector), the pointer gets invalid!!!
           Therefore, a resize() is called before the loop (no resize()-operations during the loop)! */
        mRUSpecies[i].setup(s,this); // setup this element

    }
}

ResourceUnitSpecies &ResourceUnit::resourceUnitSpecies(const Species *species)
{
    return mRUSpecies[species->index()];
}

Tree &ResourceUnit::newTree()
{
    // start simple: just append to the vector...
    mTrees.append(Tree());
    return mTrees.back();
}
int ResourceUnit::newTreeIndex()
{
    // start simple: just append to the vector...
    mTrees.append(Tree());
    return mTrees.count()-1;
}

/// remove dead trees from tree list
/// reduce size of vector if lots of space is free
/// tests showed that this way of cleanup is very fast,
/// because no memory allocations are performed (simple memmove())
/// when trees are moved.
void ResourceUnit::cleanTreeList()
{
    QVector<Tree>::iterator last=mTrees.end()-1;
    QVector<Tree>::iterator current = mTrees.begin();
    while (last>=current && (*last).isDead())
        --last;

    while (current<last) {
        if ((*current).isDead()) {
            *current = *last; // copy data!
            --last; //
            while (last>=current && (*last).isDead())
                --last;
        }
        ++current;
    }
    ++last; // last points now to the first dead tree

    // free ressources
    if (last!=mTrees.end()) {
        mTrees.erase(last, mTrees.end());
        if (mTrees.capacity()>100) {
            if (mTrees.count() / double(mTrees.capacity()) < 0.2) {
                int target_size = mTrees.count()*2;
                qDebug() << "reduce size from "<<mTrees.capacity() << "to" << target_size;
                mTrees.reserve(qMax(target_size, 100));
            }
        }
    }
}

void ResourceUnit::newYear()
{
    mAggregatedWLA = 0.;
    mAggregatedLA = 0.;
    mAggregatedLR = 0.;
    mEffectiveArea = 0.;
    mPixelCount = mStockedPixelCount = 0;
    // clear statistics global and per species...
    ResourceUnitSpecies *i;
    QVector<ResourceUnitSpecies>::iterator iend = mRUSpecies.end();
    mStatistics.clear();
    for (i=mRUSpecies.begin(); i!=iend; ++i) {
        i->statisticsDead().clear();
        i->statisticsMgmt().clear();
    }

}

/** production() is the "stand-level" part of the biomass production (3PG).
    - The amount of radiation intercepted by the stand is calculated
    - the water cycle is calculated
    - statistics for each species are cleared
    - The 3PG production for each species and ressource unit is called (calculates species-responses and NPP production)
    see also: http://iland.boku.ac.at/individual+tree+light+availability */
void ResourceUnit::production()
{

    if (mAggregatedWLA==0 || mPixelCount==0) {
        // nothing to do...
        return;
    }

    // the pixel counters are filled during the height-grid-calculations
    mStockedArea = 100. * mStockedPixelCount; // m2 (1 height grid pixel = 10x10m)

    // calculate the leaf area index (LAI)
    double LAI = mAggregatedLA / mStockedArea;
    // calculate the intercepted radiation fraction using the law of Beer Lambert
    const double k = Model::settings().lightExtinctionCoefficient;
    double interception_fraction = 1. - exp(-k * LAI);
    mEffectiveArea = mStockedArea * interception_fraction; // m2

    // calculate the total weighted leaf area on this RU:
    mLRI_modification = interception_fraction *  mStockedArea / mAggregatedWLA;
    if (mLRI_modification == 0.)
        qDebug() << "lri modifaction==0!";


    DBGMODE(qDebug() << QString("production: LAI: %1 (intercepted fraction: %2, stocked area: %4). LRI-Multiplier: %3")
            .arg(LAI)
            .arg(interception_fraction)
            .arg(mLRI_modification)
            .arg(mStockedArea);
    );

    // calculate LAI fractions
    ResourceUnitSpecies *i;
    QVector<ResourceUnitSpecies>::iterator iend = mRUSpecies.end();
    for (i=mRUSpecies.begin(); i!=iend; ++i) {
         i->setLAIfactor(i->statistics().leafAreaIndex() / leafAreaIndex());
    }

    // soil water model - this determines soil water contents needed for response calculations
    {
    DebugTimer tw("water:run");
    mWater->run();
    }

    // invoke species specific calculation (3PG)
    for (i=mRUSpecies.begin(); i!=iend; ++i) {
        i->calculate();
        if (logLevelInfo() &&  i->LAIfactor()>0)
            qDebug() << "ru" << mIndex << "species" << (*i).species()->id() << "LAIfraction" << i->LAIfactor() << "raw_gpp_m2"
                     << i->prod3PG().GPPperArea() << "area:" << productiveArea() << "gpp:"
                     << productiveArea()*i->prod3PG().GPPperArea()
                     << "aging(lastyear):" << averageAging() << "f_env,yr:" << i->prod3PG().fEnvYear();
    }
}

void ResourceUnit::calculateInterceptedArea()
{
    if (mAggregatedLR==0) {
        mEffectiveArea_perWLA = 0.;
        return;
    }
    Q_ASSERT(mAggregatedLR>0.);
    mEffectiveArea_perWLA = mEffectiveArea / mAggregatedLR;
    if (logLevelDebug()) qDebug() << "RU: aggregated lightresponse:" << mAggregatedLR  << "eff.area./wla:" << mEffectiveArea_perWLA;
}

// function is called immediately before the growth of individuals
void ResourceUnit::beforeGrow()
{
    mAverageAging = 0.;
}

// function is called after finishing the indivdual growth / mortality.
void ResourceUnit::afterGrow()
{
    mAverageAging = leafArea()>0.?mAverageAging/leafArea():0; // calculate aging value (calls to addAverageAging() by individual trees)
    if (mAverageAging>0. && mAverageAging<0.00001)
        qDebug() << "ru" << mIndex << "aging <0.00001";
}

void ResourceUnit::yearEnd()
{
    // calculate statistics for all tree species of the ressource unit
    int c = mRUSpecies.count();
    for (int i=0;i<c; i++) {
        mRUSpecies[i].statisticsDead().calculate(); // calculate the dead trees
        mRUSpecies[i].statisticsMgmt().calculate(); // stats of removed trees
        mRUSpecies[i].updateGWL(); // get sum of dead trees (died + removed)
        mRUSpecies[i].statistics().calculate(); // calculate the living (and add removed volume to gwl)
        mStatistics.add(mRUSpecies[i].statistics());
    }
    mStatistics.calculate(); // aggreagte on stand level
}

/// refresh of tree based statistics.
void ResourceUnit::createStandStatistics()
{
    // clear statistics (ru-level and ru-species level)
    mStatistics.clear();
    for (int i=0;i<mRUSpecies.count();i++) {
        mRUSpecies[i].statistics().clear();
        mRUSpecies[i].statisticsDead().clear();
        mRUSpecies[i].statisticsMgmt().clear();
    }

    // add all trees to the statistics objects of the species
    foreach(const Tree &t, mTrees) {
        if (!t.isDead())
            resourceUnitSpecies(t.species()).statistics().add(&t, 0);
    }
    // summarize statistics for the whole resource unit
    for (int i=0;i<mRUSpecies.count();i++) {
        mRUSpecies[i].statistics().calculate();
        mStatistics.add(mRUSpecies[i].statistics());
    }
    mStatistics.calculate();
    mAverageAging = mStatistics.leafAreaIndex()>0.?mAverageAging / (mStatistics.leafAreaIndex()*area()):0.;
}

void ResourceUnit::setSaplingHeightAt(const QPoint &position, const float height)
{
    Q_ASSERT(mSaplingHeightMap);
    int pixel_index = cPxPerRU*(position.x()-mCornerCoord.x())+(position.y()-mCornerCoord.y());
    if (pixel_index<0 || pixel_index>=cPxPerRU*cPxPerRU)
        qDebug() << "setSaplingHeightAt-Error";
    else
        mSaplingHeightMap[pixel_index]=height;
}

