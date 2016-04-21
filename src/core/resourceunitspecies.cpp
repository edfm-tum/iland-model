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
    mStatistics.setResourceUnitSpecies(this);
    mStatisticsDead.setResourceUnitSpecies(this);
    mStatisticsMgmt.setResourceUnitSpecies(this);

    mRemovedGrowth = 0.;
    mLastYear = -1;

    DBGMODE( if(mSpecies->index()>1000 || mSpecies->index()<0)
             qDebug() << "suspicious species?? in RUS::setup()";
                );

}


void ResourceUnitSpecies::calculate(const bool fromEstablishment)
{

    // if *not* called from establishment, clear the species-level-stats
    if (!fromEstablishment)
        statistics().clear();

    // if already processed in this year, do not repeat
    if (mLastYear == GlobalSettings::instance()->currentYear())
        return;

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
            out << mEstablishment.TACAfrostDaysAfterBudBirst() << mEstablishment.waterLimitation() << mEstablishment.abioticEnvironment();
            out << m3PG.fEnvYear() << mEstablishment.avgLIFValue() << mEstablishment.numberEstablished();
            out << mSaplingStat.livingSaplings() << mSaplingStat.averageHeight() << mSaplingStat.averageAge() << mSaplingStat.averageDeltaHPot() << mSaplingStat.averageDeltaHRealized();
            out << mSaplingStat.newSaplings() << mSaplingStat.diedSaplings() << mSaplingStat.recruitedSaplings() << mSpecies->saplingGrowthParameters().referenceRatio;
        }
    //); // DBGMODE()


    if ( logLevelDebug() )
        qDebug() << "establishment of RU" << mRU->index() << "species" << species()->id()
        << "seeds density:" << mEstablishment.avgSeedDensity()
        << "abiotic environment:" << mEstablishment.abioticEnvironment()
        << "f_env,yr:" << m3PG.fEnvYear()
        << "N(established):" << mEstablishment.numberEstablished();

}



