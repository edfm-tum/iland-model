#include "global.h"
#include "resourceunitspecies.h"

#include "species.h"
#include "resourceunit.h"
#include "model.h"
#include "watercycle.h"
#include "debugtimer.h"

/** @class ResourceUnitSpecies
  @ingroup core
    The class contains data available at ResourceUnit x Species scale.
    Data stored is either statistical (i.e. number of trees per species) or used
    within the model (e.g. fraction of utilizable Radiation).
    Important submodules are:
    * 3PG production (Production3PG)
    * Establishment
    * Growth and Recruitment of Saplings
    * Snag dynamics
  */
ResourceUnitSpecies::~ResourceUnitSpecies()
{
}

double ResourceUnitSpecies::leafArea() const
{
    // Leaf area of the species:
    // total leaf area on the RU * fraction of leafarea
    return mLAIfactor * ru()->leafAreaIndex();
}

void ResourceUnitSpecies::setup(Species *species, ResourceUnit *ru)
{
    mSpecies = species;
    mRU = ru;
    mResponse.setup(this);
    m3PG.setResponse(&mResponse);
    mEstablishment.setup(ru->climate(), this);
    mSapling.setup(this);
    mStatistics.setResourceUnitSpecies(this);
    mStatisticsDead.setResourceUnitSpecies(this);
    mStatisticsMgmt.setResourceUnitSpecies(this);

    mRemovedGrowth = 0.;
    mLastYear = -1;

}


void ResourceUnitSpecies::calculate(const bool fromEstablishment)
{

    if (mLastYear == GlobalSettings::instance()->currentYear())
        return;

    // if *not* called from establishment, clear the species-level-stats
    if (!fromEstablishment)
        statistics().clear();

    if (mLAIfactor>0. || fromEstablishment==true) {
        // execute the water calculation...
        if (fromEstablishment)
            const_cast<WaterCycle*>(mRU->waterCycle())->run(); // run the water sub model (only if this has not be done already)
        DebugTimer rst("response+3pg");
        mResponse.calculate();// calculate environmental responses per species (vpd, temperature, ...)
        m3PG.calculate();// production of NPP
        mLastYear = GlobalSettings::instance()->currentYear(); // mark this year as processed
    } else {
        // if no LAI is present, then just clear the respones.
        mResponse.clear();
        m3PG.clear();
    }
}


void ResourceUnitSpecies::updateGWL()
{
    // removed growth is the running sum of all removed
    // tree volume. the current "GWL" therefore is current volume (standing) + mRemovedGrowth.
    // important: statisticsDead() and statisticsMgmt() need to calculate() before -> volume() is already scaled to ha
    mRemovedGrowth+=statisticsDead().volume() + statisticsMgmt().volume();
}

void ResourceUnitSpecies::calculateEstablishment()
{
    //DebugTimer t("to remove");
    mEstablishment.calculate();
    //qDebug() << species()->id() << t.elapsed() << mEstablishment.avgSeedDensity() << mEstablishment.numberEstablished()<< "new";
    //DBGMODE(
        if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dEstablishment)) {
            DebugList &out = GlobalSettings::instance()->debugList(ru()->index(), GlobalSettings::dEstablishment);
            // establishment details
            out << mSpecies->id() << ru()->index() << ru()->id();
            out << mEstablishment.avgSeedDensity();
            out << mEstablishment.TACAminTemp() << mEstablishment.TACAchill() << mEstablishment.TACAfrostFree() << mEstablishment.TACgdd();
            out << mEstablishment.TACAfrostDaysAfterBudBirst() << mEstablishment.abioticEnvironment();
            out << m3PG.fEnvYear() << mEstablishment.avgLIFValue() << mEstablishment.numberEstablished();
            out << mSapling.livingSaplings() << mSapling.averageHeight() << mSapling.averageAge() << mSapling.averageDeltaHPot() << mSapling.averageDeltaHRealized();
            out << mSapling.newSaplings() << mSapling.diedSaplings() << mSapling.recruitedSaplings() << mSpecies->saplingGrowthParameters().referenceRatio;
        }
    //); // DBGMODE()


    if ( logLevelDebug() )
        qDebug() << "establishment of RU" << mRU->index() << "species" << species()->id()
        << "seeds density:" << mEstablishment.avgSeedDensity()
        << "abiotic environment:" << mEstablishment.abioticEnvironment()
        << "f_env,yr:" << m3PG.fEnvYear()
        << "N(established):" << mEstablishment.numberEstablished();

}

void ResourceUnitSpecies::calclulateSaplingGrowth()
{
    mSapling.calculateGrowth();
}

void ResourceUnitSpecies::visualGrid(Grid<float> &grid) const
{
    mSapling.fillMaxHeightGrid(grid);
}


