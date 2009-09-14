/** @class Species encapsulates a single tree species.
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
    QString stampFile = stringVar("LIPFile");
    // load stamps
    mLIPs.load( GlobalSettings::instance()->path(stampFile, "lip") );
    // attach writer stamps to reader stamps
    mLIPs.attachReaderStamps(mSet->readerStamps());

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

    // turnover rates
    mTurnoverLeaf = doubleVar("turnoverLeaf");
    mTurnoverRoot = doubleVar("turnoverRoot");

    // hd-relations
    mHDlow.setAndParse(stringVar("HDlow"));
    mHDhigh.setAndParse(stringVar("HDhigh"));

    // form/density
    mWoodDensity = doubleVar("woodDensity");
    mFormFactor = doubleVar("formFactor");
    // volume = formfactor*pi/4 *d^2*h -> volume = volumefactor * d^2 * h
    mVolumeFactor = mFormFactor * M_PI_4;

    // mortality
    // the probabilites (mDeathProb_...) are the yearly prob. of death.
    // from a population a fraction of p_lucky remains after ageMax years.
    double p_lucky = doubleVar("ProbIntrinsic");
    double age_max = doubleVar("AgeMax");
    double p_lucky_stress = doubleVar("ProbStress");
    if (p_lucky * age_max * p_lucky_stress == 0.) {
        throw IException( QString("Error setting up species %1: invalid mortality parameters.").arg(id()));
    }

    mDeathProb_intrinsic = 1. - pow(p_lucky, 1. / age_max);
    mDeathProb_stress = 1. - pow(p_lucky_stress, 0.1); // 10 years (after 10 stress years (full stress), p_lucky_stress percent survive

    if (mFoliage_a*mFoliage_b*mRoot_a*mRoot_b*mWoody_a*mWoody_b*mBranch_a*mBranch_b*mWoodDensity*mFormFactor*mSpecificLeafArea == 0.) {
        throw IException( QString("Error setting up species %1: one value is NULL in database.").arg(id()));
    }
}

double Species::biomassFoliage(const double dbh) const
{
    return mFoliage_a * pow(dbh, mFoliage_b);
}

double Species::biomassWoody(const double dbh) const
{
    return mWoody_a * pow(dbh, mWoody_b);
}

double Species::biomassRoot(const double dbh) const
{
    return mRoot_a * pow(dbh, mRoot_b);
}

/** calculate fraction of stem wood increment base on dbh.
    allometric equation: a*d^b -> first derivation: a*b*d^(b-1)
    the ratio for stem is 1 minus the ratio of twigs to total woody increment at current "dbh". */
double Species::allometricFractionStem(const double dbh) const
{
    double fraction_stem = 1. - (mBranch_a*mBranch_b*pow(dbh, mBranch_b-1.)) / (mWoody_a*mWoody_b*pow(dbh, mWoody_b-1));
    return fraction_stem;
}



