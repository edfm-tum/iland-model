#include "establishment.h"

#include "climate.h"
#include "species.h"
#include "resourceunitspecies.h"

struct EstablishmentParameters
{
    double min_temp; //degC
    int chill_requirement; // days of chilling requirement
    int GDD_min, GDD_max; // GDD thresholds
    double GDD_baseTemperature; // for GDD-calc: GDD=sum(T - baseTemp)
    int bud_birst; // GDDs needed until bud burst
    int frost_free; // minimum number of annual frost-free days required
    double frost_tolerance; //factor in growing season frost tolerance calculation
    EstablishmentParameters(): min_temp(-37), chill_requirement(56), GDD_min(177), GDD_max(3261), GDD_baseTemperature(3.4),
                               bud_birst(255), frost_free(65), frost_tolerance(0.5) {}
};

/** @class Establishment
    Establishment deals with the establishment process of saplings.
    http://iland.boku.ac.at/establishment
    Prerequisites for establishment are:
    the availability of seeds: derived from the seed-maps per Species (@sa SeedDispersal)
    the quality of the abiotic environment (TACA-model): calculations are performend here, based on climate and species responses
    the quality of the biotic environment, mainly light: based on the LIF-values

  */
Establishment::Establishment(const Climate *climate, const ResourceUnitSpecies *rus)
{
    mClimate = climate;
    mRUS = rus;
    mRegenerationProbability = 0.;
    mPAbiotic = 0.;
}

void Establishment::calculate()
{
    calculateAbioticEnvironment();
}


/** Calculate the abiotic environemnt for seedling for a given species and a given resource unit.
 The model is closely based on the TACA approach of Nitschke and Innes (2008), Ecol. Model 210, 263-277
 more details: http://iland.boku.ac.at/establishment#abiotic_environment
 a model mockup in R: script_establishment.r

 */
void Establishment::calculateAbioticEnvironment()
{
    EstablishmentParameters p; // should then be part of the species parameter set
    const Phenology &pheno = mClimate->phenology(mRUS->species()->phenologyClass());

    bool p_min_temp = true; // minimum temperature threshold
    bool p_chill = false;  // (total) chilling requirement
    bool p_gdd = false;   // gdd-thresholds
    bool p_frost_free = false; // frost free days in vegetation period


    const ClimateDay *day = mClimate->begin();
    int doy = 0;
    double GDD=0.;
    double GDD_BudBirst = 0.;
    int chill_days = pheno.chillingDaysLastYear(); // chilling days of the last autumn
    int frost_free = 0;
    int frost_after_bud = 0;
    bool chill_ok = false;
    bool buds_are_birst = false;
    for (; day!=mClimate->end(); ++day, ++doy) {
        // minimum temperature: if temp too low -> set prob. to zero
        if (day->min_temperature < p.min_temp)
            p_min_temp = false;

        // chilling requirement, GDD, bud birst
        if (day->temperature>=-5. && day->temperature<5.)
            chill_days++;
        if (chill_days>p.chill_requirement)
            chill_ok=true;
        // GDDs above the base temperature are counted if beginning from the day where the chilling requirements are met
        // up to a fixed day ending the veg period
        if (doy>=pheno.vegetationPeriodStart() && doy<=pheno.vegetationPeriodEnd()) {
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

            if (buds_are_birst && day->min_temperature <= 0.)
                frost_after_bud++;

            // count frost free days
            if (day->min_temperature > 0.)
                frost_free++;
        }
    }
    // chilling requirement
    if (pheno.chillingDays() > p.chill_requirement)
        p_chill = true;

    // GDD requirements
    if (GDD>p.GDD_min && GDD<p.GDD_max)
        p_gdd = true;

    // frost free days in the vegetation period
    if (frost_free > p.frost_free)
        p_frost_free = true;

    // if all requirements are met:
    if (p_chill && p_min_temp & p_gdd & p_frost_free) {
        // negative effect of frost events after bud birst
        double frost_effect = pow(p.frost_tolerance, sqrt(double(frost_after_bud)));
        mPAbiotic = frost_effect;
    } else {
        mPAbiotic = 0.; // if any of the requirements is not met
    }

}
