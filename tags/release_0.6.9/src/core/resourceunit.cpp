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
#include "snag.h"
#include "soil.h"
#include "helper.h"

ResourceUnit::~ResourceUnit()
{
    if (mWater)
        delete mWater;
    mWater = 0;
    if (mSnag)
        delete mSnag;
    if (mSoil)
        delete mSoil;

    mSnag = 0;
    mSoil = 0;
}

ResourceUnit::ResourceUnit(const int index)
{
    qDeleteAll(mRUSpecies);
    mSpeciesSet = 0;
    mClimate = 0;
    mPixelCount=0;
    mStockedArea = 0;
    mStockedPixelCount = 0;
    mIndex = index;
    mSaplingHeightMap = 0;
    mEffectiveArea_perWLA = 0.;
    mWater = new WaterCycle();
    mSnag = 0;
    mSoil = 0;
    mID = 0;
}

void ResourceUnit::setup()
{
    mWater->setup(this);

    if (mSnag)
        delete mSnag;
    mSnag=0;
    if (mSoil)
        delete mSoil;
    mSoil=0;
    if (Model::settings().carbonCycleEnabled) {
        mSoil = new Soil(this);
        mSnag = new Snag;
        mSnag->setup(this);
        const XmlHelper &xml=GlobalSettings::instance()->settings();

        // setup contents of the soil of the RU; use values for C and N (kg/ha)
        mSoil->setInitialState(CNPool(xml.valueDouble("model.site.youngLabileC", -1),
                                      xml.valueDouble("model.site.youngLabileN", -1),
                                      xml.valueDouble("model.site.youngLabileDecompRate", -1)),
                               CNPool(xml.valueDouble("model.site.youngRefractoryC", -1),
                                      xml.valueDouble("model.site.youngRefractoryN", -1),
                                      xml.valueDouble("model.site.youngRefractoryDecompRate", -1)),
                               CNPair(xml.valueDouble("model.site.somC", -1), xml.valueDouble("model.site.somN", -1)));
    }

    // setup variables
    mUnitVariables.nitrogenAvailable = GlobalSettings::instance()->settings().valueDouble("model.site.availableNitrogen", 40);

    // if dynamic coupling of soil nitrogen is enabled, the calculate a starting value for available n.
    if (mSoil && Model::settings().useDynamicAvailableNitrogen && Model::settings().carbonCycleEnabled) {
        mSoil->setClimateFactor(1.);
        mSoil->calculateYear();
        mUnitVariables.nitrogenAvailable = mSoil->availableNitrogen();
    }
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
    qDeleteAll(mRUSpecies);

    //mRUSpecies.resize(set->count()); // ensure that the vector space is not relocated
    for (int i=0;i<set->count();i++) {
        Species *s = const_cast<Species*>(mSpeciesSet->species(i));
        if (!s)
            throw IException("ResourceUnit::setSpeciesSet: invalid index!");

        ResourceUnitSpecies *rus = new ResourceUnitSpecies();
        mRUSpecies.push_back(rus);
        rus->setup(s, this);
        /* be careful: setup() is called with a pointer somewhere to the content of the mRUSpecies container.
           If the container memory is relocated (QVector), the pointer gets invalid!!!
           Therefore, a resize() is called before the loop (no resize()-operations during the loop)! */
        //mRUSpecies[i].setup(s,this); // setup this element

    }
}

ResourceUnitSpecies &ResourceUnit::resourceUnitSpecies(const Species *species)
{
    return *mRUSpecies[species->index()];
}

Tree &ResourceUnit::newTree()
{
    // start simple: just append to the vector...
    if (mTrees.isEmpty())
        mTrees.reserve(100); // reserve a junk of memory for trees

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
                //int target_size = mTrees.count()*2;
                //qDebug() << "reduce size from "<<mTrees.capacity() << "to" << target_size;
                //mTrees.reserve(qMax(target_size, 100));
                qDebug() << "reduce tree storage of RU" << index() << " from " << mTrees.capacity() << "to" << mTrees.count();
                mTrees.squeeze();
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
    snagNewYear();
    if (mSoil)
        mSoil->newYear();
    // clear statistics global and per species...
    QList<ResourceUnitSpecies*>::const_iterator i;
    QList<ResourceUnitSpecies*>::const_iterator iend = mRUSpecies.constEnd();
    mStatistics.clear();
    for (i=mRUSpecies.constBegin(); i!=iend; ++i) {
        (*i)->statisticsDead().clear();
        (*i)->statisticsMgmt().clear();
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
    mLRI_modification = interception_fraction *  mStockedArea / mAggregatedWLA; // p_WLA
    if (mLRI_modification == 0.)
        qDebug() << "lri modifaction==0!";

    if (logLevelDebug()) {
    DBGMODE(qDebug() << QString("production: LAI: %1 (intercepted fraction: %2, stocked area: %4). LRI-Multiplier: %3")
            .arg(LAI)
            .arg(interception_fraction)
            .arg(mLRI_modification)
            .arg(mStockedArea);
    );
    }

    // calculate LAI fractions
    QList<ResourceUnitSpecies*>::const_iterator i;
    QList<ResourceUnitSpecies*>::const_iterator iend = mRUSpecies.constEnd();
    double ru_lai = leafAreaIndex();
    if (ru_lai < 1.)
        ru_lai = 1.;
    // note: LAIFactors are only 1 if sum of LAI is > 1. (see WaterCycle)
    for (i=mRUSpecies.constBegin(); i!=iend; ++i) {
         (*i)->setLAIfactor((*i)->statistics().leafAreaIndex() / ru_lai);
    }

    // soil water model - this determines soil water contents needed for response calculations
    {
    mWater->run();
    }

    // invoke species specific calculation (3PG)
    for (i=mRUSpecies.constBegin(); i!=iend; ++i) {
        (*i)->calculate(); // CALCULATE 3PG
        if (logLevelInfo() &&  (*i)->LAIfactor()>0)
            qDebug() << "ru" << mIndex << "species" << (*i)->species()->id() << "LAIfraction" << (*i)->LAIfactor() << "raw_gpp_m2"
                     << (*i)->prod3PG().GPPperArea() << "area:" << productiveArea() << "gpp:"
                     << productiveArea()*(*i)->prod3PG().GPPperArea()
                     << "aging(lastyear):" << averageAging() << "f_env,yr:" << (*i)->prod3PG().fEnvYear();
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
    if (mAverageAging<0. || mAverageAging>1.)
        qDebug() << "Average aging invalid: (RU, LAI):" << index() << mStatistics.leafAreaIndex();
}

void ResourceUnit::yearEnd()
{
    // calculate statistics for all tree species of the ressource unit
    int c = mRUSpecies.count();
    for (int i=0;i<c; i++) {
        mRUSpecies[i]->statisticsDead().calculate(); // calculate the dead trees
        mRUSpecies[i]->statisticsMgmt().calculate(); // stats of removed trees
        mRUSpecies[i]->updateGWL(); // get sum of dead trees (died + removed)
        mRUSpecies[i]->statistics().calculate(); // calculate the living (and add removed volume to gwl)
        mStatistics.add(mRUSpecies[i]->statistics());
    }
    mStatistics.calculate(); // aggreagte on stand level

}

void ResourceUnit::addTreeAgingForAllTrees()
{
    mAverageAging = 0.;
    foreach(const Tree &t, mTrees) {
        addTreeAging(t.leafArea(), t.species()->aging(t.height(), t.age()));
    }

}

/// refresh of tree based statistics.
/// WARNING: this function is only called once (during startup).
/// see function "yearEnd()" above!!!
void ResourceUnit::createStandStatistics()
{
    // clear statistics (ru-level and ru-species level)
    mStatistics.clear();
    for (int i=0;i<mRUSpecies.count();i++) {
        mRUSpecies[i]->statistics().clear();
        mRUSpecies[i]->statisticsDead().clear();
        mRUSpecies[i]->statisticsMgmt().clear();
    }

    // add all trees to the statistics objects of the species
    foreach(const Tree &t, mTrees) {
        if (!t.isDead())
            resourceUnitSpecies(t.species()).statistics().add(&t, 0);
    }
    // summarize statistics for the whole resource unit
    for (int i=0;i<mRUSpecies.count();i++) {
        mRUSpecies[i]->statistics().calculate();
        mStatistics.add(mRUSpecies[i]->statistics());
    }
    mStatistics.calculate();
    mAverageAging = mStatistics.leafAreaIndex()>0.?mAverageAging / (mStatistics.leafAreaIndex()*stockableArea()):0.;
    if (mAverageAging<0. || mAverageAging>1.)
        qDebug() << "Average aging invalid: (RU, LAI):" << index() << mStatistics.leafAreaIndex();
}

void ResourceUnit::setMaxSaplingHeightAt(const QPoint &position, const float height)
{
    Q_ASSERT(mSaplingHeightMap);
    int pixel_index = cPxPerRU*(position.x()-mCornerCoord.x())+(position.y()-mCornerCoord.y());
    if (pixel_index<0 || pixel_index>=cPxPerRU*cPxPerRU) {
        qDebug() << "setSaplingHeightAt-Error for position" << position << "for RU at" << boundingBox() << "with corner" << mCornerCoord;
    } else {
        if (mSaplingHeightMap[pixel_index]<height)
            mSaplingHeightMap[pixel_index]=height;
    }
}

/// clear all saplings of all species on a given position (after recruitment)
void ResourceUnit::clearSaplings(const QPoint &position)
{
    foreach(ResourceUnitSpecies* rus, mRUSpecies)
        rus->clearSaplings(position);

}

float ResourceUnit::saplingHeightForInit(const QPoint &position) const
{
    double maxh = 0.;
    foreach(ResourceUnitSpecies* rus, mRUSpecies)
        maxh = qMax(maxh, rus->sapling().heightAt(position));
    return maxh;
}

void ResourceUnit::calculateCarbonCycle()
{
    if (!snag())
        return;

    // (1) calculate the snag dynamics
    // because all carbon/nitrogen-flows from trees to the soil are routed through the snag-layer,
    // all soil inputs (litter + deadwood) are collected in the Snag-object.
    snag()->calculateYear();
    soil()->setClimateFactor( snag()->climateFactor() ); // the climate factor is only calculated once
    soil()->setSoilInput( snag()->labileFlux(), snag()->refractoryFlux());
    soil()->calculateYear(); // update the ICBM/2N model
    // use available nitrogen?
    if (Model::settings().useDynamicAvailableNitrogen)
        mUnitVariables.nitrogenAvailable = soil()->availableNitrogen();

    // debug output
    if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dCarbonCycle) && !snag()->isEmpty()) {
        DebugList &out = GlobalSettings::instance()->debugList(index(), GlobalSettings::dCarbonCycle);
        out << index() << id(); // resource unit index and id
        out << snag()->debugList(); // snag debug outs
        out << soil()->debugList(); // ICBM/2N debug outs
    }

}

