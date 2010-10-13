#include "establishment.h"

#include "climate.h"
#include "species.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "seeddispersal.h"
#include "model.h"


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
    mRegenerationProbability = 0.;
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
    mRegenerationProbability = 0.;
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
    if (mRUS->ru()->saplingHeightAt(pos_lif) > 1.3)
        return false;

    // check if sapling of the current tree species is already established -> if so, no establishment.
    if (mRUS->hasSaplingAt(pos_lif))
        return false;


    double h_height_grid = GlobalSettings::instance()->model()->heightGrid()->valueAtIndex(pos_lif.x()/cPxPerHeight, pos_lif.y()/cPxPerHeight).height;
    if (h_height_grid==0)
        throw IException(QString("establishTree: height grid at %1/%2 has value 0").arg(pos_lif.x()).arg(pos_lif.y()));
    double rel_height = 4. / h_height_grid;

     double lif_corrected = mRUS->species()->speciesSet()->LRIcorrection(lif_value, rel_height);
     float p_est = mPAbiotic * lif_corrected * seed_value;
     // statistics
     mLIFcount++;
     mSumLIFvalue+=lif_value;
     // draw a random number and check against the combined establishment probability
     double p_rand = drandom(mRUS->ru()->randomGenerator());
     if (p_rand < p_est) {
         const_cast<ResourceUnitSpecies*>(mRUS)->addSapling(pos_lif);
         return true; // establishment
     }
     return false; // no establishment
}

// see http://iland.boku.ac.at/establishment
void Establishment::calculate()
{
    mPAbiotic = 0.;
    mNumberEstablished = 0;
    mPxDensity = 0.;
    mTACA_chill=mTACA_frostfree=mTACA_gdd=mTACA_gdd=mTACA_min_temp=false;
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
    if (with_seeds==0)
        return;

    // 2nd step: environmental drivers
    calculateAbioticEnvironment();
    if (mPAbiotic == 0.)
        return;

    // the effect of water, nitrogen, co2, .... is a bulk factor: f_env,yr
    const_cast<ResourceUnitSpecies*>(mRUS)->calculate(true); // calculate the 3pg module (only if that not done already)
    double f_env_yr = mRUS->prod3PG().fEnvYear();
    mPAbiotic *= f_env_yr;
    if (mPAbiotic == 0.)
        return;

    int n_established = 0;
    // 3rd step: check actual pixels in the LIF grid
    if (with_seeds/double(total) > 0.4 ) {
        // a large part has available seeds. simply scan the pixels...
        QPoint lif_index;
        Grid<float> &lif_map = *GlobalSettings::instance()->model()->grid();
        GridRunner<float> lif_runner(lif_map, ru_rect);
        while (float *lif_px = lif_runner.next()) {
            lif_index = lif_map.indexOf(lif_px);
            if (!ru_rect.contains(lif_map.cellCenterPoint(lif_index)))
                qDebug() << "(a) establish problem:" << lif_index << "point: " << lif_map.cellCenterPoint(lif_index) << "not in" << ru_rect;
            // value of the seed map: seed_map.valueAt( lif_map.cellCenterPoint(lif_map.indexOf(lif_px)) );
            if (establishTree(lif_index, *lif_px, seed_map.constValueAt( lif_map.cellCenterPoint(lif_index) )))
                n_established++;
        }

    } else {
        // relatively few seed-pixels are filled. So examine seed pixels first, and check light only on "filled" pixels
        GridRunner<float> seed_runner(seed_map, ru_rect);
        Grid<float> &lif_map = *GlobalSettings::instance()->model()->grid();
        // check every pixel inside the bounding box of the pixel with
        while (float *p = seed_runner.next()) {
            if (*p>0.f) {
                // pixel with seeds: now really iterate over lif pixels
                GridRunner<float> lif_runner(lif_map, seed_map.cellRect(seed_map.indexOf(p)));
                while (float *lif_px = lif_runner.next()) {
                    if (!ru_rect.contains(lif_map.cellCenterPoint(lif_map.indexOf(lif_px))))
                        qDebug() << "(b) establish problem:" << lif_map.indexOf(lif_px) << "point: " << lif_map.cellCenterPoint(lif_map.indexOf(lif_px)) << "not in" << ru_rect;
                    if (establishTree(lif_map.indexOf(lif_px), *lif_px ,*p))
                        n_established++;
                }
            }
        }
    }
    // finished!!!
    mNumberEstablished = n_established;
}



/** Calculate the abiotic environemnt for seedling for a given species and a given resource unit.
 The model is closely based on the TACA approach of Nitschke and Innes (2008), Ecol. Model 210, 263-277
 more details: http://iland.boku.ac.at/establishment#abiotic_environment
 a model mockup in R: script_establishment.r

 */
void Establishment::calculateAbioticEnvironment()
{
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
    if (mTACA_chill && mTACA_min_temp & mTACA_gdd & mTACA_frostfree) {
        // negative effect of frost events after bud birst
        double frost_effect = 1.;
        if (mTACA_frostAfterBuds>0)
            frost_effect = pow(p.frost_tolerance, sqrt(double(mTACA_frostAfterBuds)));
        mPAbiotic = frost_effect;
    } else {
        mPAbiotic = 0.; // if any of the requirements is not met
    }

}
