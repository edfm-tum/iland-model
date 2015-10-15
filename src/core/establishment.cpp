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

#include "establishment.h"

#include "climate.h"
#include "species.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "seeddispersal.h"
#include "model.h"
#include "grasscover.h"
#include "helper.h"
#include "debugtimer.h"

/** @class Establishment
    Establishment deals with the establishment process of saplings.
    http://iland.boku.ac.at/establishment
    Prerequisites for establishment are:
    the availability of seeds: derived from the seed-maps per Species (@sa SeedDispersal)
    the quality of the abiotic environment (TACA-model): calculations are performend here, based on climate and species responses
    the quality of the biotic environment, mainly light: based on the LIF-values

  */
Establishment::Establishment()
{
    mPAbiotic = 0.;
}

Establishment::Establishment(const Climate *climate, const ResourceUnitSpecies *rus)
{
    setup(climate, rus);
}

void Establishment::setup(const Climate *climate, const ResourceUnitSpecies *rus)
{
    mClimate = climate;
    mRUS = rus;
    mPAbiotic = 0.;
    mPxDensity = 0.;
    mNumberEstablished = 0;
    if (climate==0 || rus==0 || rus->species()==0 || rus->ru()==0)
        throw IException("Establishment::setup: important variable is null.");
}

inline bool Establishment::establishTree(const QPoint &pos_lif, const float lif_value, const float seed_value)
{

    // check window of opportunity...if regeneration (of any species) on the current pixel is above breast height (1.3m), then
    // no establishment is possible
    if (mRUS->ru()->saplingHeightAt(pos_lif) > 1.3f)
        return false;

    // check if sapling of the current tree species is already established -> if so, no establishment.
    if (mRUS->hasSaplingAt(pos_lif))
        return false;

    const HeightGridValue &hgv = GlobalSettings::instance()->model()->heightGrid()->constValueAtIndex(pos_lif.x()/cPxPerHeight, pos_lif.y()/cPxPerHeight);
    // no establishment if pixel is not in project area
    if (!hgv.isValid())
        return false;

    double h_height_grid = hgv.height;
    if (h_height_grid==0)
        throw IException(QString("establishTree: height grid at %1/%2 has value 0").arg(pos_lif.x()).arg(pos_lif.y()));
    double rel_height = 4. / h_height_grid;

     double lif_corrected = mRUS->species()->speciesSet()->LRIcorrection(lif_value, rel_height);
     float p_est = lif_corrected * seed_value;
     // orig: with *mPAbiotic ::: now moved to one level above
     // float p_est = mPAbiotic * lif_corrected * seed_value;
     // 2011-11-09: it is important to use a *new* random number: this is then mathematically the same as
     // multiplying probabilties.
     // statistics
     mLIFcount++;
     mSumLIFvalue+=lif_value;
     // draw a random number and check against the combined establishment probability
     double p_rand = drandom();
     if (p_rand < p_est) {
         const_cast<ResourceUnitSpecies*>(mRUS)->addSapling(pos_lif);
         return true; // establishment
     }
     return false; // no establishment
}

int _est_old_algo=0;
int _est_new_algo=0;

// see http://iland.boku.ac.at/establishment
void Establishment::calculate()
{
    //DebugTimer t("est_calculate"); t.setSilent();

    mPAbiotic = 0.;
    mNumberEstablished = 0;
    mPxDensity = 0.;
    mTACA_min_temp=mTACA_chill=mTACA_gdd=mTACA_frostfree=false;
    mTACA_frostAfterBuds=0;
    mSumLIFvalue = 0.;
    mLIFcount = 0;

    // Step 1: determine, whether there are seeds in the current resource unit
    const Grid<float> &seed_map = mRUS->species()->seedDispersal()->seedMap();
    const QRectF &ru_rect = mRUS->ru()->boundingBox();
    GridRunner<float> runner(seed_map, ru_rect);
    // check every pixel inside the bounding box of the resource unit
    int with_seeds = 0, total=0;
    while (float *p = runner.next()) {
        if (*p>0.f) {
            with_seeds++;
        }
        mPxDensity += *p;
        total++;
    }
    if (total==0)
        throw IException("Establishment: number of seed map pixels on resource unit " + QString::number(mRUS->ru()->index()) + " is 0.");
    mPxDensity /= double(total);
    //mPxDensity = with_seeds / double(total);
    if (with_seeds==0)
        return;


    // 2nd step: environmental drivers
    calculateAbioticEnvironment();
    if (mPAbiotic == 0.)
        return;


    // the effect of water, nitrogen, co2, .... is a bulk factor: f_env,yr
    const_cast<ResourceUnitSpecies*>(mRUS)->calculate(true); // calculate the 3pg module (only if that is not already done)
    double f_env_yr = mRUS->prod3PG().fEnvYear();
    mPAbiotic *= f_env_yr;
    if (mPAbiotic == 0.)
        return;

    // switch the algorithm:
    // use the new algorithm, if most of the area is covered with seed pixels

    if (with_seeds/double(total) < 0.9) {
        calculatePerSeedPixel();     // the original algorithm (started in 2010 (approx.), and tinkered with later on)
        ++_est_old_algo;
    } else {
        calculatePerRU(); // the new one (2015) - works best if seeds are available everywhere
        ++_est_new_algo;
    }

}

void Establishment::debugInfo()
{
    qDebug() << "Establisment: new algo:"  << _est_new_algo << "old algo:" << _est_old_algo;
}



/** Calculate the abiotic environemnt for seedling for a given species and a given resource unit.
 The model is closely based on the TACA approach of Nitschke and Innes (2008), Ecol. Model 210, 263-277
 more details: http://iland.boku.ac.at/establishment#abiotic_environment
 a model mockup in R: script_establishment.r

 */
void Establishment::calculateAbioticEnvironment()
{
    //DebugTimer t("est_abiotic"); t.setSilent();

    const EstablishmentParameters &p = mRUS->species()->establishmentParameters();
    const Phenology &pheno = mClimate->phenology(mRUS->species()->phenologyClass());

    mTACA_min_temp = true; // minimum temperature threshold
    mTACA_chill = false;  // (total) chilling requirement
    mTACA_gdd = false;   // gdd-thresholds
    mTACA_frostfree = false; // frost free days in vegetation period
    mTACA_frostAfterBuds = 0; // frost days after bud birst


    const ClimateDay *day = mClimate->begin();
    int doy = 0;
    double GDD=0.;
    double GDD_BudBirst = 0.;
    int chill_days = pheno.chillingDaysLastYear(); // chilling days of the last autumn
    int frost_free = 0;
    mTACA_frostAfterBuds = 0;
    bool chill_ok = false;
    bool buds_are_birst = false;
    int veg_period_end = pheno.vegetationPeriodEnd();
    if (veg_period_end >= 365)
        veg_period_end = mClimate->sun().dayShorter10_5hrs();

    for (; day!=mClimate->end(); ++day, ++doy) {
        // minimum temperature: if temp too low -> set prob. to zero
        if (day->min_temperature < p.min_temp)
            mTACA_min_temp = false;

        // count frost free days
        if (day->min_temperature > 0.)
            frost_free++;

        // chilling requirement, GDD, bud birst
        if (day->temperature>=-5. && day->temperature<5.)
            chill_days++;
        if (chill_days>p.chill_requirement)
            chill_ok=true;
        // GDDs above the base temperature are counted if beginning from the day where the chilling requirements are met
        // up to a fixed day ending the veg period
        if (doy<=veg_period_end) {
            // accumulate growing degree days
            if (chill_ok && day->temperature > p.GDD_baseTemperature) {
                GDD += day->temperature - p.GDD_baseTemperature;
                GDD_BudBirst += day->temperature - p.GDD_baseTemperature;
            }
            // if day-frost occurs, the GDD counter for bud birst is reset
            if (day->temperature <= 0.)
                GDD_BudBirst = 0.;

            if (GDD_BudBirst > p.bud_birst)
                buds_are_birst = true;

            if (doy<veg_period_end && buds_are_birst && day->min_temperature <= 0.)
                mTACA_frostAfterBuds++;
        }
    }
    // chilling requirement
    if (chill_ok)
        mTACA_chill = true;

    // GDD requirements
    if (GDD>p.GDD_min && GDD<p.GDD_max)
        mTACA_gdd = true;

    // frost free days in the vegetation period
    if (frost_free > p.frost_free)
        mTACA_frostfree = true;

    // if all requirements are met:
    if (mTACA_chill && mTACA_min_temp && mTACA_gdd && mTACA_frostfree) {
        // negative effect of frost events after bud birst
        double frost_effect = 1.;
        if (mTACA_frostAfterBuds>0)
            frost_effect = pow(p.frost_tolerance, sqrt(double(mTACA_frostAfterBuds)));
        mPAbiotic = frost_effect;
    } else {
        mPAbiotic = 0.; // if any of the requirements is not met
    }

}

void Establishment::calculatePerSeedPixel()
{

    int n_established = 0;
    const Grid<float> &seed_map = mRUS->species()->seedDispersal()->seedMap();
    const QRectF &ru_rect = mRUS->ru()->boundingBox();

    GridRunner<float> seed_runner(seed_map, ru_rect);
    Model *model = GlobalSettings::instance()->model();
    Grid<float> *lif_map = model->grid();
    // check every pixel inside the bounding box of the pixel with
    while (float *p = seed_runner.next()) {
        if (*p>0.f) {

            if (*p>100.) { //(*p<0.01): OK this seems also to change the results....
                // select a pixel randomly, but increase probability by the same factor
                float mod_p = *p * 100;
                QPoint lif_index = lif_map->indexAt(seed_runner.currentCoord());
                lif_index += QPoint( irandom(-5,4), irandom(-5,4) );
                double p_establish = drandom();
                if (p_establish < mPAbiotic) {
                    if (establishTree(lif_index, lif_map->constValueAtIndex(lif_index) ,mod_p))
                        n_established++;
                }


            } else {
                //double p_establish = drandom(mRUS->ru()->randomGenerator());
                //if (p_establish > mPAbiotic)
                //    continue;
                // pixel with seeds: now really iterate over lif pixels
                GridRunner<float> lif_runner(lif_map, seed_map.cellRect(seed_runner.currentIndex()));
                while (float *lif_px = lif_runner.next()) {
                    DBGMODE(
                                if (!ru_rect.contains(lif_map->cellCenterPoint(lif_map->indexOf(lif_px))))
                                qDebug() << "(b) establish problem:" << lif_map->indexOf(lif_px) << "point: " << lif_map->cellCenterPoint(lif_map->indexOf(lif_px)) << "not in" << ru_rect;
                            );
                    double p_establish = drandom();
                    QPoint lif_index = lif_map->indexOf(lif_px);
                    double grass_cover = 1. - model->grassCover()->regenerationInhibition(lif_index);
                    if (p_establish < mPAbiotic*grass_cover) {
                        if (establishTree(lif_index, *lif_px ,*p))
                            n_established++;
                    }
                }
            }
        }
    }

    mNumberEstablished = n_established;

}

void Establishment::calculatePerRU()
{
    int n_established = 0;

    // accessed grids
    Grid<float> *lif_map = GlobalSettings::instance()->model()->grid();
    const Grid<float> &seed_map = mRUS->species()->seedDispersal()->seedMap();

    const QRectF &ru_rect = mRUS->ru()->boundingBox();

    // the bit set has true of every 2x2m position that is already occupied by
    // a sapling of the current species
    const std::bitset<cPxPerRU*cPxPerRU> &pos_bitset = mRUS->sapling().presentPositions();

    GridRunner<float> lif_runner(lif_map, ru_rect);

    const float *sap_height = mRUS->ru()->saplingHeightMapPointer();
    size_t bit_idx=0;
    Model *model = GlobalSettings::instance()->model();
    while (float *lif_px = lif_runner.next()) {

        // check for height of sapling < 1.3m (for all species
        // and for presence of a sapling of the given species
        if (*sap_height++ >=1.3f || pos_bitset[bit_idx++])
            continue;

        QPoint lif_index = lif_map->indexOf(lif_px);

        // check the abiotic environment against a random number
        double grass_cover = 1. - model->grassCover()->regenerationInhibition(lif_index);
        if (grass_cover == 0.)
            continue; // no chance of establishment

        double p = drandom();
        if (p > mPAbiotic * grass_cover)
            continue;

        const float seed_map_value = seed_map.constValueAt( lif_runner.currentCoord() );

        // check the light availability at that pixel
        const HeightGridValue &hgv = model->heightGrid()->constValueAtIndex(lif_index.x()/cPxPerHeight, lif_index.y()/cPxPerHeight);
        // no establishment if pixel is not in project area
        if (!hgv.isValid())
            continue;


        double h_height_grid = hgv.height;
        double rel_height = 4. / h_height_grid;

        double lif_corrected = mRUS->species()->speciesSet()->LRIcorrection(*lif_px, rel_height);

        mLIFcount++;
        mSumLIFvalue+=*lif_px;

        // check for the combination of seed availability and light on the forest floor
        if (drandom() < seed_map_value*lif_corrected ) {
            // ok, a new tree should be established - we do not use the establishTree() function
            const_cast<ResourceUnitSpecies*>(mRUS)->addSapling(lif_index);
            n_established++;
        }

    }
    mNumberEstablished = n_established;
}
