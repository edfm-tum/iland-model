#include "snag.h"
#include "tree.h"
#include "species.h"
#include "globalsettings.h"
#include "expression.h"

/** @class Snag
  Snag deals with carbon / nitrogen fluxes from the forest until the reach soil pools.
  Snag lives on the level of RU x species; carbon fluxes from trees enter Snag, and parts of the biomass of snags
  is subsequently forwarded to the soil sub model.

  */
// static variables
double Snag::mDBHLower = 10.;
double Snag::mDBHHigher = 30.;
double CNPool::biomassCFraction = biomassCFraction; // get global from globalsettings.h

// species specific soil paramters
struct SoilParameters
{
    SoilParameters(): kyl(0.15), kyr(0.0807), ksw(0.015), cnFoliage(75.), cnFineroot(40.), cnWood(300.) {}
    double kyl; // litter decomposition rate
    double kyr; // downed woody debris (dwd) decomposition rate
    double ksw; // standing woody debris (swd) decomposition rate
    double cnFoliage; //  C/N foliage litter
    double cnFineroot; // C/N ratio fine root
    double cnWood; // C/N Wood: used for brances, stem and coarse root
    Expression pDWDformula; // expression that calculates annual transition probability for standing to downed deadwood. variable: 'tsd' (time since death)
} soilparams;


Snag::Snag()
{
    soilparams.pDWDformula.setExpression("1-1/(1+exp(-6.78+0.262*tsd))");
}

void Snag::setup()
{
    // branches
    mBranchCounter=0;
    for (int i=0;i<3;i++) {
        mTimeSinceDeath[i] = 0.;
        mNumberOfSnags[i] = 0.;
    }
    mTotalSnagCarbon = 0.;
}

// debug outputs
QList<QVariant> Snag::debugList()
{
    // list columns
    // for three pools
    QList<QVariant> list;

    list << mTotalSnagCarbon;

    for (int i=0;i<3;i++) {
        // pools "swdx_c", "swdx_n", "swdx_count", "swdx_tsd", "toswdx_c", "toswdx_n"
        list << mSWD[i].C << mSWD[i].N << mNumberOfSnags[i] << mTimeSinceDeath[i] << mToSWD[i].C << mToSWD[i].N;
    }

    // fluxes to labile soil pool and to refractory soil pool
    list << mLabileFlux.C << mLabileFlux.N << mRefractoryFlux.C << mRefractoryFlux.N;

    // branch pools (5 yrs)
    list << mBranches[mBranchCounter].C << mBranches[mBranchCounter].N
            << mBranches[(mBranchCounter+1)%5].C << mBranches[(mBranchCounter+1)%5].N
            << mBranches[(mBranchCounter+2)%5].C << mBranches[(mBranchCounter+2)%5].N
            << mBranches[(mBranchCounter+3)%5].C << mBranches[(mBranchCounter+3)%5].N
            << mBranches[(mBranchCounter+4)%5].C << mBranches[(mBranchCounter+4)%5].N;
    return list;
}

void Snag::newYear()
{
    for (int i=0;i<3;i++) {
        mToSWD[i].clear(); // clear transfer pools to standing-woody-debris
    }
    mLabileFlux.clear();
    mRefractoryFlux.clear();
}

// do the yearly calculation
// see http://iland.boku.ac.at/snag+dynamics
void Snag::processYear()
{
    if (mLabileFlux.isEmpty() && mRefractoryFlux.isEmpty() && isEmpty()) // nothing to do
        return;

    const SoilParameters &soil_params = soilparams; // to be updated

    // process branches: every year one of the five baskets is emptied and transfered to the refractory soil pool
    mRefractoryFlux+=mBranches[mBranchCounter];
    mBranches[mBranchCounter].clear();
    mBranchCounter= (mBranchCounter+1) % 5; // increase index, roll over to 0.

    // process standing snags.
    // the input of the current year is in the mToSWD-Pools
    double tsd;
    const double climate_factor_re = 0.5; // todo
    for (int i=0;i<3;i++) {
        // calculate 'tsd', i.e. time-since-death (see SnagDecay.xls for calculation details)
        // time_since_death = tsd_last_year*state_before / (state_before+input) + 1
        if (mSWD[i].C>0.)
            tsd = mTimeSinceDeath[i]*mSWD[i].C / (mSWD[i].C+mToSWD[i].C) + 1.;
        else
            tsd = 0.;

        // update the swd-pool: move content to the SWD pool
        mSWD[i] += mToSWD[i];
        mToSWD[i].clear();

        mTimeSinceDeath[i] = tsd;

        if (mSWD[i].C>0.) {
            // calculate decay of SWD.
            // Eq. (1): mass (C) is lost, N remains unchanged.
            double factor = exp(-soil_params.ksw * climate_factor_re * 1.);
            mSWD[i].C *= factor;

            // calculate the transition probability of SWD to downed dead wood
            double pDWD = soil_params.pDWDformula.calculate(tsd);
            pDWD = limit( pDWD * climate_factor_re, 0., 1.); //  modified transition rate with climate decomp factor
            // calculate flow to soil pool...
            mRefractoryFlux += mSWD[i] * pDWD;
            mSWD[i] *= (1.-pDWD); // reduce pool
            // calculate the stem number of remaining snags
            mNumberOfSnags[i] = mNumberOfSnags[i] * (1. - pDWD);
            if (mNumberOfSnags[i] < 0.5) {
                // clear the pool: add the rest to the soil
                mRefractoryFlux += mSWD[i];
                mSWD[i].clear();
            }
        }
    }
    // total carbon in the snag *after* processing is the content of the
    // standing woody debris pools + the branches
    mTotalSnagCarbon = mSWD[0].C + mSWD[1].C + mSWD[2].C +
                       mBranches[0].C + mBranches[1].C + mBranches[2].C + mBranches[3].C + mBranches[4].C;
}

/// foliage and fineroot litter is transferred during tree growth.
void Snag::addTurnoverLitter(const Tree *tree, const double litter_foliage, const double litter_fineroot)
{
    const SoilParameters &soil_params = soilparams; // to be updated
    mLabileFlux.addBiomass(litter_foliage, soil_params.cnFoliage);
    mLabileFlux.addBiomass(litter_fineroot, soil_params.cnFineroot);
}

/// after the death of the tree the five biomass compartments are processed.
void Snag::addMortality(const Tree *tree)
{
    const SoilParameters &soil_params = soilparams; // to be updated

    // immediate flows: 100% of foliage, 100% of fine roots: they go to the labile pool
    // 100% of coarse root biomass: that goes to the refractory pool
    mLabileFlux.addBiomass(tree->biomassFoliage(), soil_params.cnFoliage);
    mLabileFlux.addBiomass(tree->biomassFineRoot(), soil_params.cnFineroot);
    mRefractoryFlux.addBiomass(tree->biomassCoarseRoot(), soil_params.cnWood);

    // branches are equally distributed over five years:
    for (int i=0;i<5; i++)
        mBranches[i].addBiomass(tree->biomassBranch() * 0.2, soil_params.cnWood);

    // stem biomass is transferred to the standing woody debris pool (SWD), increase stem number of pool
    CNPool &swd = mToSWD[poolIndex(tree->dbh())]; // get right transfer pool
    swd.addBiomass(tree->biomassStem(), soil_params.cnWood);
    mNumberOfSnags[poolIndex(tree->dbh())]++;
}

/// add residual biomass of 'tree' after harvesting.
/// remove_(stem, branch, foliage)_fraction: percentage of biomass compartment that is *removed* by the harvest operation.
/// the harvested biomass is collected.
void Snag::addHarvest(const Tree* tree, const double remove_stem_fraction, const double remove_branch_fraction, const double remove_foliage_fraction )
{
    const SoilParameters &soil_params = soilparams; // to be updated

    // immediate flows: 100% of residual foliage, 100% of fine roots: they go to the labile pool
    // 100% of coarse root biomass: that goes to the refractory pool
    mLabileFlux.addBiomass(tree->biomassFoliage() * (1. - remove_foliage_fraction), soil_params.cnFoliage);
    mLabileFlux.addBiomass(tree->biomassFineRoot(), soil_params.cnFineroot);
    mRefractoryFlux.addBiomass(tree->biomassCoarseRoot(), soil_params.cnWood);

    // residual branches are equally distributed over five years:
    for (int i=0;i<5; i++)
        mBranches[i].addBiomass(tree->biomassBranch() * remove_branch_fraction * 0.2, soil_params.cnWood);

    // stem biomass is transferred to the standing woody debris pool (SWD), increase stem number of pool
    CNPool &swd = mToSWD[poolIndex(tree->dbh())]; // get right transfer pool
    swd.addBiomass(tree->biomassStem() * remove_stem_fraction, soil_params.cnWood);
    if (remove_stem_fraction < 1.)
        mNumberOfSnags[poolIndex(tree->dbh())]++;
}

