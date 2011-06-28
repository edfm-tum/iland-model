/** @class Species
  The behavior and general properties of tree species.
  Because the individual trees are designed as leightweight as possible, lots of stuff is done by the Species.
  Inter alia, Species do:
  - store all the precalcualted patterns for light competition (LIP, stamps)
  - do most of the growth (3PG) calculation
  */


#include <QtCore>
#include "globalsettings.h"

#include "species.h"
#include "speciesset.h"
#include "stampcontainer.h"
#include "exception.h"
#include "seeddispersal.h"


Species::~Species()
{
    if (mSeedDispersal)
        delete mSeedDispersal;
}

/** main setup routine for tree species.
  Data is fetched from the open query (or file, ...) in the parent SpeciesSet using xyzVar() functions.
  This is called
*/
void Species::setup()
{
    Q_ASSERT(mSet != 0);
    // setup general information
    mId = stringVar("shortName");
    mName = stringVar("name");
    QString col_name = '#' + stringVar("displayColor");
    mDisplayColor.setNamedColor(col_name);
    QString stampFile = stringVar("LIPFile");
    // load stamps
    mLIPs.load( GlobalSettings::instance()->path(stampFile, "lip") );
    // attach writer stamps to reader stamps
    mLIPs.attachReaderStamps(mSet->readerStamps());
    // general properties
    mConiferous = boolVar("isConiferous");
    mEvergreen = boolVar("isEvergreen");

    // setup allometries
    mFoliage_a = doubleVar("bmFoliage_a");
    mFoliage_b = doubleVar("bmFoliage_b");

    mWoody_a = doubleVar("bmWoody_a");
    mWoody_b = doubleVar("bmWoody_b");

    mRoot_a = doubleVar("bmRoot_a");
    mRoot_b = doubleVar("bmRoot_b");

    mBranch_a = doubleVar("bmBranch_a");
    mBranch_b = doubleVar("bmBranch_b");

    mSpecificLeafArea = doubleVar("specificLeafArea");
    mFinerootFoliageRatio = doubleVar("finerootFoliageRatio");

    // cn-ratios
    mCNFoliage = doubleVar("cnFoliage");
    mCNFineroot = doubleVar("cnFineRoot");
    mCNWood = doubleVar("cnWood");

    // turnover rates
    mTurnoverLeaf = doubleVar("turnoverLeaf");
    mTurnoverRoot = doubleVar("turnoverRoot");

    // hd-relations
    mHDlow.setAndParse(stringVar("HDlow"));
    mHDhigh.setAndParse(stringVar("HDhigh"));
    mHDlow.linearize(0., 100.); // input: dbh (cm). above 100cm the formula will be directly executed
    mHDhigh.linearize(0., 100.);

    // form/density
    mWoodDensity = doubleVar("woodDensity");
    mFormFactor = doubleVar("formFactor");
    // volume = formfactor*pi/4 *d^2*h -> volume = volumefactor * d^2 * h
    mVolumeFactor = mFormFactor * M_PI_4;

    // snags
    mSnagKSW = doubleVar("snagKSW"); // decay rate of SWD
    mSnagHalflife = doubleVar("snagHalfLife");
    mSnagKYL = doubleVar("snagKYL"); // decay rate labile
    mSnagKYR = doubleVar("snagKYR"); // decay rate refractory matter

    if (mFoliage_a*mFoliage_b*mRoot_a*mRoot_b*mWoody_a*mWoody_b*mBranch_a*mBranch_b*mWoodDensity*mFormFactor*mSpecificLeafArea*mFinerootFoliageRatio == 0.) {
        throw IException( QString("Error setting up species %1: one value is NULL in database.").arg(id()));
    }
    // Aging
    mMaximumAge = doubleVar("maximumAge");
    mMaximumHeight = doubleVar("maximumHeight");
    mAging.setAndParse(stringVar("aging"));
    mAging.linearize(0.,1.); // input is harmonic mean of relative age and relative height
    if (mMaximumAge*mMaximumHeight==0)
        throw IException( QString("Error setting up species %1:invalid aging parameters.").arg(id()));

    // mortality
    // the probabilites (mDeathProb_...) are the yearly prob. of death.
    // from a population a fraction of p_lucky remains after ageMax years. see wiki: base+mortality
    double p_lucky = doubleVar("probIntrinsic");
    double p_lucky_stress = doubleVar("probStress");

    if (p_lucky * mMaximumAge * p_lucky_stress == 0.) {
        throw IException( QString("Error setting up species %1: invalid mortality parameters.").arg(id()));
    }

    mDeathProb_intrinsic = 1. - pow(p_lucky, 1. / mMaximumAge);
    mDeathProb_stress = p_lucky_stress;

    if (logLevelInfo()) qDebug() << "species" << name() << "probStress" << p_lucky_stress << "resulting probability:" << mDeathProb_stress;

    // envirionmental responses
    mRespVpdExponent = doubleVar("respVpdExponent");
    mRespTempMin  =doubleVar("respTempMin");
    mRespTempMax  =doubleVar("respTempMax");
    if (mRespVpdExponent>=0) throw IException( QString("Error: vpd exponent >=0 for species (must be a negative value).").arg(id()));
    if (mRespTempMax==0. || mRespTempMin>=mRespTempMax) throw IException( QString("temperature response parameters invalid for species").arg(id()));

    mRespNitrogenClass = doubleVar("respNitrogenClass");
    if (mRespNitrogenClass<1 || mRespNitrogenClass>3) throw IException( QString("nitrogen class invalid (must be >=1 and <=3) for species").arg(id()));

    // phenology
    mPhenologyClass = (int)doubleVar("phenologyClass");

    // water
    mMaxCanopyConductance = doubleVar("maxCanopyConductance");
    mPsiMin = -fabs(doubleVar("psiMin")); // force a negative value

    // light
    mLightResponseClass = doubleVar("lightResponseClass");
    if (mLightResponseClass<1. || mLightResponseClass>5.)
        throw IException( QString("invalid light response class for species %1. Allowed: 1..5.").arg(id()));

    // regeneration
    int seed_year_interval = intVar("seedYearInterval");
    if (seed_year_interval==0)
        throw IException(QString("seedYearInterval = 0 for %1").arg(id()));
    mSeedYearProbability = 1 / (double)seed_year_interval;
    mMaturityYears = intVar("maturityYears");
    mTM_as1 = doubleVar("seedKernel_as1");
    mTM_as2 = doubleVar("seedKernel_as2");
    mTM_ks = doubleVar("seedKernel_ks0");
    mFecundity_m2 = doubleVar("fecundity_m2");
    mNonSeedYearFraction = doubleVar("nonSeedYearFraction");

    // establishment parameters
    mEstablishmentParams.min_temp = doubleVar("estMinTemp");
    mEstablishmentParams.chill_requirement = intVar("estChillRequirement");
    mEstablishmentParams.GDD_min = intVar("estGDDMin");
    mEstablishmentParams.GDD_max = intVar("estGDDMax");
    mEstablishmentParams.GDD_baseTemperature = doubleVar("estGDDBaseTemp");
    mEstablishmentParams.bud_birst = intVar("estBudBirstGDD");
    mEstablishmentParams.frost_free = intVar("estFrostFreeDays");
    mEstablishmentParams.frost_tolerance = doubleVar("estFrostTolerance");

    // sapling and sapling growth parameters
    mSaplingGrowthParams.heightGrowthPotential.setAndParse(stringVar("sapHeightGrowthPotential"));
    mSaplingGrowthParams.heightGrowthPotential.linearize(0., 1.);
    mSaplingGrowthParams.hdSapling = doubleVar("sapHDSapling");
    mSaplingGrowthParams.stressThreshold = doubleVar("sapStressThreshold");
    mSaplingGrowthParams.maxStressYears = intVar("sapMaxStressYears");
    mSaplingGrowthParams.referenceRatio = doubleVar("sapReferenceRatio");
    mSaplingGrowthParams.ReinekesR = doubleVar("sapReinekesR");

}


/** calculate fraction of stem wood increment base on dbh.
    allometric equation: a*d^b -> first derivation: a*b*d^(b-1)
    the ratio for stem is 1 minus the ratio of twigs to total woody increment at current "dbh". */
double Species::allometricFractionStem(const double dbh) const
{
    double fraction_stem = 1. - (mBranch_a*mBranch_b*pow(dbh, mBranch_b-1.)) / (mWoody_a*mWoody_b*pow(dbh, mWoody_b-1));
    return fraction_stem;
}

/** Aging formula.
   calculates a relative "age" by combining a height- and an age-related term using a harmonic mean,
   and feeding this into the Landsberg and Waring formula.
   see http://iland.boku.ac.at/primary+production#respiration_and_aging
   @param useAge set to true if "real" tree age is available. If false, only the tree height is used.
  */
double Species::aging(const float height, const int age) const
{
    double rel_height = qMin(height/mMaximumHeight, 0.999999); // 0.999999 -> avoid div/0
    double rel_age = qMin(age/mMaximumAge, 0.999999);

    // harmonic mean: http://en.wikipedia.org/wiki/Harmonic_mean
    double x = 1. - 2. / (1./(1.-rel_height) + 1./(1.-rel_age)); // Note:

    double aging_factor = mAging.calculate(x);

    return limit(aging_factor, 0., 1.); // limit to [0..1]
}

int Species::estimateAge(const float height) const
{
    int age_rel = int( mMaximumAge * height / mMaximumHeight );
    return age_rel;
}

/** Seed production.
   This function produces seeds if the tree is older than a species-specific age ("maturity")
   If seeds are produced, this information is stored in a "SeedMap"
  */
void Species::seedProduction(const int age, const float height, const QPoint &position_index)
{
    if (!mSeedDispersal)
        return; // regeneration is disabled
    // no seed production if maturity age is not reached (species parameter) or if tree height is below 4m.
    if (age > mMaturityYears && height > 4.f) {
        mSeedDispersal->setMatureTree(position_index);
    }
}


/** newYear is called by the SpeciesSet at the beginning of a year before any growth occurs.
  This is used for various initializations, e.g. to clear seed dispersal maps
  */
void Species::newYear()
{
    if (seedDispersal()) {
        // decide whether current year is a seed year
        mIsSeedYear = (drandom() < mSeedYearProbability);
        if (mIsSeedYear)
            qDebug() << "species" << id() << "has a seed year.";
        // clear seed map
        seedDispersal()->clear();
    }
}