#include "snag.h"
#include "tree.h"
#include "species.h"
#include "globalsettings.h"
#include "expression.h"
// for calculation of climate decomposition
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"
#include "model.h"

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

double CNPair::biomassCFraction = biomassCFraction; // get global from globalsettings.h

/// add biomass and weigh the parameter_value with the current C-content of the pool
void CNPool::addBiomass(const double biomass, const double CNratio, const double parameter_value)
{
    if (biomass==0.)
        return;
    double new_c = biomass*biomassCFraction;
    double p_old = C / (new_c + C);
    mParameter = mParameter*p_old + parameter_value*(1.-p_old);
    CNPair::addBiomass(biomass, CNratio);
}

// increase pool (and weigh the value)
void CNPool::operator+=(const CNPool &s)
{
    if (s.C==0.)
        return;
    mParameter = parameter(s); // calculate weighted parameter
    C+=s.C;
    N+=s.N;
}

double CNPool::parameter(const CNPool &s) const
{
    if (s.C==0.)
        return parameter();
    double p_old = C / (s.C + C);
    double result =  mParameter*p_old + s.parameter()*(1.-p_old);
    return result;
}


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
    CNPair::setCFraction(biomassCFraction);
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
        mHalfLife[i] = 0.;
    }
    mTotalSnagCarbon = 0.;
    if (mDBHLower<=0)
        throw IException("Snag::setupThresholds() not called or called with invalid parameters.");

    // Inital values from XML file
    XmlHelper xml=GlobalSettings::instance()->settings();
    double kyr = xml.valueDouble("model.site.youngRefractoryDecompRate", -1);
    // put carbon of snags to the middle size class
    xml.setCurrentNode("model.initialization.snags");
    mSWD[1].C = xml.valueDouble(".swdC");
    mSWD[1].N = mSWD[1].C / xml.valueDouble(".swdCN", 50.);
    mSWD[1].setParameter(kyr);
    mKSW[1] = xml.valueDouble(".swdDecompRate");
    mNumberOfSnags[1] = xml.valueDouble(".swdCount");
    mHalfLife[1] = xml.valueDouble(".swdHalfLife");
    // and for the Branch/coarse root pools: split the init value into five chunks
    CNPool other(xml.valueDouble(".otherC"), xml.valueDouble(".otherC")/xml.valueDouble(".otherCN", 50.), kyr );

    mTotalSnagCarbon = other.C + mSWD[1].C;

    other *= 0.2;
    for (int i=0;i<5;i++)
        mOtherWood[i] = other;
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

    // branch/coarse wood pools (5 yrs)
    for (int i=0;i<5;i++) {
        list << mOtherWood[i].C << mOtherWood[i].N;
    }
//    list << mOtherWood[mBranchCounter].C << mOtherWood[mBranchCounter].N
//            << mOtherWood[(mBranchCounter+1)%5].C << mOtherWood[(mBranchCounter+1)%5].N
//            << mOtherWood[(mBranchCounter+2)%5].C << mOtherWood[(mBranchCounter+2)%5].N
//            << mOtherWood[(mBranchCounter+3)%5].C << mOtherWood[(mBranchCounter+3)%5].N
//            << mOtherWood[(mBranchCounter+4)%5].C << mOtherWood[(mBranchCounter+4)%5].N;
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
    mTotalToDisturbance.clear();
    mTotalIn.clear();
    mSWDtoSoil.clear();
}

/// calculate the dynamic climate modifier for decomposition 're'
/// calculation is done on the level of ResourceUnit because the water content per day is needed.
double Snag::calculateClimateFactors()
{
    double ft, fw;
    double f_sum = 0.;
    int iday=0;
    // calculate the water-factor for each month (see Adair et al 2008)
    double fw_month[12];
    double ratio;
    for (int m=0;m<12;m++) {
        if (mRU->waterCycle()->referenceEvapotranspiration()[m]>0.)
            ratio = mRU->climate()->precipitationMonth()[m] /  mRU->waterCycle()->referenceEvapotranspiration()[m];
        else
            ratio = 0;
        fw_month[m] = 1. / (1. + 30.*exp(-8.5*ratio));
        if (logLevelDebug()) qDebug() <<"month"<< m << "PET" << mRU->waterCycle()->referenceEvapotranspiration()[m] << "prec" <<mRU->climate()->precipitationMonth()[m];
    }

    for (const ClimateDay *day=mRU->climate()->begin(); day!=mRU->climate()->end(); ++day, ++iday)
    {
        ft = exp(308.56*(1./56.02-1./((273.+day->temperature)-227.13)));  // empirical variable Q10 model of Lloyd and Taylor (1994), see also Adair et al. (2008)
        fw = fw_month[day->month-1];

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
    const double climate_factor_re = calculateClimateFactors(); // calculate anyway, because also the soil module needs it (and currently one can have Snag and Soil only as a couple)
    if (isEmpty()) // nothing to do
        return;

    // process branches: every year one of the five baskets is emptied and transfered to the refractory soil pool
    mRefractoryFlux+=mOtherWood[mBranchCounter];

    mOtherWood[mBranchCounter].clear();
    mBranchCounter= (mBranchCounter+1) % 5; // increase index, roll over to 0.
    // decay of branches/coarse roots
    for (int i=0;i<5;i++) {
        if (mOtherWood[i].C>0.) {
            double survive_rate = exp(- climate_factor_re * mOtherWood[i].parameter() ); // parameter: the "kyr" value...
            mOtherWood[i].C *= survive_rate;
        }
    }

    // process standing snags.
    // the input of the current year is in the mToSWD-Pools
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
                       mOtherWood[0].C + mOtherWood[1].C + mOtherWood[2].C + mOtherWood[3].C + mOtherWood[4].C;
    mTotalSWD = mSWD[0] + mSWD[1] + mSWD[2];
    mTotalOther = mOtherWood[0] + mOtherWood[1] + mOtherWood[2] + mOtherWood[3] + mOtherWood[4];
}

/// foliage and fineroot litter is transferred during tree growth.
void Snag::addTurnoverLitter(const Species *species, const double litter_foliage, const double litter_fineroot)
{
    mLabileFlux.addBiomass(litter_foliage, species->cnFoliage(), species->snagKyl());
    mLabileFlux.addBiomass(litter_fineroot, species->cnFineroot(), species->snagKyl());
}

void Snag::addTurnoverWood(const Species *species, const double woody_biomass)
{
    mRefractoryFlux.addBiomass(woody_biomass, species->cnWood(), species->snagKyr());
}

/// after the death of the tree the five biomass compartments are processed.
void Snag::addMortality(const Tree *tree)
{
    const Species *species = tree->species();

    // immediate flows: 100% of foliage, 100% of fine roots: they go to the labile pool
    mLabileFlux.addBiomass(tree->biomassFoliage(), species->cnFoliage(), tree->species()->snagKyl());
    mLabileFlux.addBiomass(tree->biomassFineRoot(), species->cnFineroot(), tree->species()->snagKyl());

    // branches and coarse roots are equally distributed over five years:
    double biomass_rest = (tree->biomassBranch()+tree->biomassCoarseRoot()) * 0.2;
    for (int i=0;i<5; i++)
        mOtherWood[i].addBiomass(biomass_rest, species->cnWood(), tree->species()->snagKyr());

    // just for book-keeping: keep track of all inputs into branches / roots / swd
    mTotalIn.addBiomass(tree->biomassBranch() + tree->biomassCoarseRoot() + tree->biomassStem(), species->cnWood());
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
    mCurrentKSW[pi] = mCurrentKSW[pi]*p_old + species->snagKsw() * p_new;
    mNumberOfSnags[pi]++;

    // finally add the biomass
    CNPool &to_swd = mToSWD[pi];
    to_swd.addBiomass(tree->biomassStem(), species->cnWood(), tree->species()->snagKyr());
}

/// add residual biomass of 'tree' after harvesting.
/// remove_{stem, branch, foliage}_fraction: percentage of biomass compartment that is *removed* by the harvest operation (i.e.: not to stay in the system)
/// records on harvested biomass is collected (mTotalToExtern-pool).
void Snag::addHarvest(const Tree* tree, const double remove_stem_fraction, const double remove_branch_fraction, const double remove_foliage_fraction )
{
    const Species *species = tree->species();

    // immediate flows: 100% of residual foliage, 100% of fine roots: they go to the labile pool
    mLabileFlux.addBiomass(tree->biomassFoliage() * (1. - remove_foliage_fraction), species->cnFoliage(), tree->species()->snagKyl());
    mLabileFlux.addBiomass(tree->biomassFineRoot(), species->cnFineroot(), tree->species()->snagKyl());

    // for branches, add all biomass that remains in the forest to the soil
    mRefractoryFlux.addBiomass(tree->biomassBranch()*(1.-remove_branch_fraction), species->cnWood(), tree->species()->snagKyr());
    // the same treatment for stem residuals
    mRefractoryFlux.addBiomass(tree->biomassStem() * (1. - remove_stem_fraction), species->cnWood(), tree->species()->snagKyr());

    // split the corase wood biomass into parts (slower decay)
    double biomass_rest = (tree->biomassCoarseRoot()) * 0.2;
    for (int i=0;i<5; i++)
        mOtherWood[i].addBiomass(biomass_rest, species->cnWood(), tree->species()->snagKyr());


    // for book-keeping...
    mTotalToExtern.addBiomass(tree->biomassFoliage()*remove_foliage_fraction +
                              tree->biomassBranch()*remove_branch_fraction +
                              tree->biomassStem()*remove_stem_fraction, species->cnWood());
}

// add flow from regeneration layer (dead trees) to soil
void Snag::addToSoil(const Species *species, const CNPair &woody_pool, const CNPair &litter_pool)
{
    mLabileFlux.add(litter_pool, species->snagKyl());
    mRefractoryFlux.add(woody_pool, species->snagKyr());
}

/// disturbance function: remove the fraction of 'factor' of biomass from the SWD pools; 0: remove nothing, 1: remove all
/// biomass removed by this function goes to the atmosphere
void Snag::removeCarbon(const double factor)
{
    // reduce pools of currently standing dead wood and also of pools that are added
    // during (previous) management operations of the current year
    for (int i=0;i<3;i++) {
        mTotalToDisturbance += (mSWD[i] + mToSWD[i]) * factor;
        mSWD[i] *= (1. - factor);
        mToSWD[i] *= (1. - factor);
    }

    for (int i=0;i<5;i++) {
        mTotalToDisturbance += mOtherWood[i]*factor;
        mOtherWood[i] *= (1. - factor);
    }
}


/// cut down swd (and branches) and move to soil pools
/// @param factor 0: cut 0%, 1: cut and slash 100% of the wood
void Snag::management(const double factor)
{
    if (factor<0. || factor>1.)
        throw IException(QString("Invalid factor in Snag::management: '%1'").arg(factor));
    // swd pools
    for (int i=0;i<3;i++) {
        mSWDtoSoil += mSWD[i] * factor;
        mSWD[i] *= (1. - factor);
        mSWDtoSoil += mToSWD[i] * factor;
        mToSWD[i] *= (1. - factor);
    }
    // what to do with the branches: now move also all wood to soil (note: this is note
    // very good w.r.t the coarse roots...
    for (int i=0;i<5;i++) {
        mRefractoryFlux+=mOtherWood[i]*factor;
        mOtherWood[i]*=(1. - factor);
    }

}

