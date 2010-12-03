#include "snag.h"
#include "tree.h"
#include "species.h"
#include "globalsettings.h"
#include "expression.h"
// for calculation of climate decomposition
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"

/** @class Snag
  Snag deals with carbon / nitrogen fluxes from the forest until the reach soil pools.
  Snag lives on the level of the ResourceUnit; carbon fluxes from trees enter Snag, and parts of the biomass of snags
  is subsequently forwarded to the soil sub model.
  Carbon is stored in three classes (depending on the size)
  The Snag dynamics class uses the following species parameter:
  cnFoliage, cnFineroot, cnWood, snagHalflife, snagKSW

  */
// static variables
double Snag::mDBHLower = -1.;
double Snag::mDBHHigher = 0.;
double Snag::mCarbonThreshold[] = {0., 0., 0.};

double CNPool::biomassCFraction = biomassCFraction; // get global from globalsettings.h

void Snag::setupThresholds(const double lower, const double upper)
{
    if (mDBHLower == lower)
        return;
    mDBHLower = lower;
    mDBHHigher = upper;
    mCarbonThreshold[0] = lower / 2.;
    mCarbonThreshold[1] = lower + (upper - lower)/2.;
    mCarbonThreshold[2] = upper + (upper - lower)/2.;
    //# threshold levels for emptying out the dbh-snag-classes
    //# derived from Psme woody allometry, converted to C, with a threshold level set to 10%
    //# values in kg!
    for (int i=0;i<3;i++)
        mCarbonThreshold[i] = 0.10568*pow(mCarbonThreshold[i],2.4247)*0.5*0.1;
}


Snag::Snag()
{
    mRU = 0;
    CNPool::setCFraction(biomassCFraction);
}

void Snag::setup( const ResourceUnit *ru)
{
    mRU = ru;
    mClimateFactor = 0.;
    // branches
    mBranchCounter=0;
    for (int i=0;i<3;i++) {
        mTimeSinceDeath[i] = 0.;
        mNumberOfSnags[i] = 0.;
        mAvgDbh[i] = 0.;
        mAvgHeight[i] = 0.;
        mAvgVolume[i] = 0.;
        mKSW[i] = 0.;
        mCurrentKSW[i] = 0.;
    }
    mTotalSnagCarbon = 0.;
    if (mDBHLower<=0)
        throw IException("Snag::setupThresholds() not called or called with invalid parameters.");
}

// debug outputs
QList<QVariant> Snag::debugList()
{
    // list columns
    // for three pools
    QList<QVariant> list;

    // totals
    list << mTotalSnagCarbon << mTotalIn.C << mTotalToAtm.C << mSWDtoSoil.C << mSWDtoSoil.N;
    // fluxes to labile soil pool and to refractory soil pool
    list << mLabileFlux.C << mLabileFlux.N << mRefractoryFlux.C << mRefractoryFlux.N;

    for (int i=0;i<3;i++) {
        // pools "swdx_c", "swdx_n", "swdx_count", "swdx_tsd", "toswdx_c", "toswdx_n"
        list << mSWD[i].C << mSWD[i].N << mNumberOfSnags[i] << mTimeSinceDeath[i] << mToSWD[i].C << mToSWD[i].N;
        list << mAvgDbh[i] << mAvgHeight[i] << mAvgVolume[i];
    }

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
        mCurrentKSW[i] = 0.;
    }
    mLabileFlux.clear();
    mRefractoryFlux.clear();
    mTotalToAtm.clear();
    mTotalToExtern.clear();
    mTotalIn.clear();
    mSWDtoSoil.clear();
}

/// calculate the dynamic climate modifier for decomposition 're'
/// calculation is done on the level of ResourceUnit because the water content per day is needed.
double Snag::calculateClimateFactors()
{
    double rel_wc;
    double ft, fw;
    double f_sum = 0.;
    for (const ClimateDay *day=mRU->climate()->begin(); day!=mRU->climate()->end(); ++day)
    {
        rel_wc = mRU->waterCycle()->relContent(day->day)*100.; // relative water content in per cent of the day
        ft = exp(308.56*(1./56.02-1./((273.+day->temperature)-227.13)));  // empirical variable Q10 model of Lloyd and Taylor (1994), see also Adair et al. (2008)
        fw = pow(1.-exp(-0.2*rel_wc),5.); //  #  see Standcarb for the 'stable soil' pool
        f_sum += ft*fw;
    }
    // the climate factor is defined as the arithmentic annual mean value
    mClimateFactor = f_sum / double(mRU->climate()->daysOfYear());
    return mClimateFactor;
}

/// do the yearly calculation
/// see http://iland.boku.ac.at/snag+dynamics
void Snag::calculateYear()
{
    mSWDtoSoil.clear();
    if (isEmpty()) // nothing to do
        return;

    // process branches: every year one of the five baskets is emptied and transfered to the refractory soil pool
    mRefractoryFlux+=mBranches[mBranchCounter];
    mSWDtoSoil += mBranches[mBranchCounter];
    mBranches[mBranchCounter].clear();
    mBranchCounter= (mBranchCounter+1) % 5; // increase index, roll over to 0.

    // process standing snags.
    // the input of the current year is in the mToSWD-Pools

    const double climate_factor_re = calculateClimateFactors();
    for (int i=0;i<3;i++) {

        // update the swd-pool with this years' input
        if (!mToSWD[i].isEmpty()) {
            // update decay rate (apply average yearly input to the state parameters)
            mKSW[i] = mKSW[i]*(mSWD[i].C/(mSWD[i].C+mToSWD[i].C)) + mCurrentKSW[i]*(mToSWD[i].C/(mSWD[i].C+mToSWD[i].C));
            //move content to the SWD pool
            mSWD[i] += mToSWD[i];
        }

        if (mSWD[i].C > 0) {
            // reduce the Carbon (note: the N stays, thus the CN ratio changes)
            // use the decay rate that is derived as a weighted average of all standing woody debris
            double survive_rate = exp(-mKSW[i] *climate_factor_re * 1. ); // 1: timestep
            mTotalToAtm.C += mSWD[i].C * (1. - survive_rate);
            mSWD[i].C *= survive_rate;

            // transition to downed woody debris
            // update: use negative exponential decay, species parameter: half-life
            // modified for the climatic effect on decomposition, i.e. if decomp is slower, snags stand longer and vice versa
            // this is loosely oriented on Standcarb2 (http://andrewsforest.oregonstate.edu/pubs/webdocs/models/standcarb2.htm),
            // where lag times for cohort transitions are linearly modified with re although here individual good or bad years have
            // an immediate effect, the average climatic influence should come through (and it is inherently transient)
            // note that swd.hl is species-specific, and thus a weighted average over the species in the input (=mortality)
            // needs to be calculated, followed by a weighted update of the previous swd.hl.
            // As weights here we use stem number, as the processes here pertain individual snags
            // calculate the transition probability of SWD to downed dead wood

            double half_life = mHalfLife[i] / climate_factor_re;
            double rate = -M_LN2 / half_life; // M_LN2: math. constant

            // higher decay rate for the class with smallest diameters
            if (i==0)
                rate*=2.;

            double transfer = 1. - exp(rate);

            // calculate flow to soil pool...
            mSWDtoSoil += mSWD[i] * transfer;
            mRefractoryFlux += mSWD[i] * transfer;
            mSWD[i] *= (1.-transfer); // reduce pool
            // calculate the stem number of remaining snags
            mNumberOfSnags[i] = mNumberOfSnags[i] * (1. - transfer);

            mTimeSinceDeath[i] += 1.;
            // if stems<0.5, empty the whole cohort into DWD, i.e. release the last bit of C and N and clear the stats
            // also, if the Carbon of an average snag is less than 10% of the original average tree
            // (derived from allometries for the three diameter classes), the whole cohort is emptied out to DWD
            if (mNumberOfSnags[i] < 0.5 || mSWD[i].C / mNumberOfSnags[i] < mCarbonThreshold[i]) {
                // clear the pool: add the rest to the soil, clear statistics of the pool
                mRefractoryFlux += mSWD[i];
                mSWDtoSoil += mSWD[i];
                mSWD[i].clear();
                mAvgDbh[i] = 0.;
                mAvgHeight[i] = 0.;
                mAvgVolume[i] = 0.;
                mKSW[i] = 0.;
                mCurrentKSW[i] = 0.;
                mHalfLife[i] = 0.;
                mTimeSinceDeath[i] = 0.;
            }

        }

    }
    // total carbon in the snag-container on the RU *after* processing is the content of the
    // standing woody debris pools + the branches
    mTotalSnagCarbon = mSWD[0].C + mSWD[1].C + mSWD[2].C +
                       mBranches[0].C + mBranches[1].C + mBranches[2].C + mBranches[3].C + mBranches[4].C;
}

/// foliage and fineroot litter is transferred during tree growth.
void Snag::addTurnoverLitter(const Tree *tree, const double litter_foliage, const double litter_fineroot)
{
    mLabileFlux.addBiomass(litter_foliage, tree->species()->cnFoliage());
    mLabileFlux.addBiomass(litter_fineroot, tree->species()->cnFineroot());
}

/// after the death of the tree the five biomass compartments are processed.
void Snag::addMortality(const Tree *tree)
{
    const Species *species = tree->species();

    // immediate flows: 100% of foliage, 100% of fine roots: they go to the labile pool
    // 100% of coarse root biomass: that goes to the refractory pool
    mLabileFlux.addBiomass(tree->biomassFoliage(), species->cnFoliage());
    mLabileFlux.addBiomass(tree->biomassFineRoot(), species->cnFineroot());
    mRefractoryFlux.addBiomass(tree->biomassCoarseRoot(), species->cnWood());

    // branches are equally distributed over five years:
    double biomass_branch = tree->biomassBranch() * 0.2;
    for (int i=0;i<5; i++)
        mBranches[i].addBiomass(biomass_branch, species->cnWood());

    // just for book-keeping: keep track of all inputs into branches / swd
    mTotalIn.addBiomass(tree->biomassBranch() + tree->biomassStem(), species->cnWood());
    // stem biomass is transferred to the standing woody debris pool (SWD), increase stem number of pool
    int pi = poolIndex(tree->dbh()); // get right transfer pool

    // update statistics - stemnumber-weighted averages
    // note: here the calculations are repeated for every died trees (i.e. consecutive weighting ... but delivers the same results)
    double p_old = mNumberOfSnags[pi] / (mNumberOfSnags[pi] + 1); // weighting factor for state vars (based on stem numbers)
    double p_new = 1. / (mNumberOfSnags[pi] + 1); // weighting factor for added tree (p_old + p_new = 1).
    mAvgDbh[pi] = mAvgDbh[pi]*p_old + tree->dbh()*p_new;
    mAvgHeight[pi] = mAvgHeight[pi]*p_old + tree->height()*p_new;
    mAvgVolume[pi] = mAvgVolume[pi]*p_old + tree->volume()*p_new;
    mTimeSinceDeath[pi] = mTimeSinceDeath[pi]*p_old + 1.*p_new;
    mHalfLife[pi] = mHalfLife[pi]*p_old + species->snagHalflife()* p_new;

    // average the decay rate (ksw); this is done based on the carbon content
    // aggregate all trees that die in the current year (and save weighted decay rates to CurrentKSW)
    if (tree->biomassStem()==0)
        throw IException("Snag::addMortality: tree without stem biomass!!");
    p_old = mToSWD[pi].C / (mToSWD[pi].C + tree->biomassStem()* biomassCFraction);
    p_new =tree->biomassStem()* biomassCFraction / (mToSWD[pi].C + tree->biomassStem()* biomassCFraction);
    mCurrentKSW[pi] = mCurrentKSW[pi]*p_old + species->snagKSW() * p_new;
    mNumberOfSnags[pi]++;

    // finally add the biomass
    CNPool &swd = mToSWD[pi];
    swd.addBiomass(tree->biomassStem(), species->cnWood());
}

/// add residual biomass of 'tree' after harvesting.
/// remove_{stem, branch, foliage}_fraction: percentage of biomass compartment that is *removed* by the harvest operation (i.e.: not to stay in the system)
/// records on harvested biomass is collected (mTotalToExtern-pool).
void Snag::addHarvest(const Tree* tree, const double remove_stem_fraction, const double remove_branch_fraction, const double remove_foliage_fraction )
{
    const Species *species = tree->species();

    // immediate flows: 100% of residual foliage, 100% of fine roots: they go to the labile pool
    // 100% of coarse root biomass: that goes to the refractory pool
    mLabileFlux.addBiomass(tree->biomassFoliage() * (1. - remove_foliage_fraction), species->cnFoliage());
    mLabileFlux.addBiomass(tree->biomassFineRoot(), species->cnFineroot());
    mRefractoryFlux.addBiomass(tree->biomassCoarseRoot(), species->cnWood());
    // for branches, add all biomass that remains in the forest to the soil
    mRefractoryFlux.addBiomass(tree->biomassBranch()*(1.-remove_branch_fraction), species->cnWood());
    // the same treatment for stem residuals
    mRefractoryFlux.addBiomass(tree->biomassStem() * remove_stem_fraction, species->cnWood());

    // for book-keeping...
    mTotalToExtern.addBiomass(tree->biomassFoliage()*remove_foliage_fraction +
                              tree->biomassBranch()*remove_branch_fraction +
                              tree->biomassStem()*remove_stem_fraction, species->cnWood());
}

