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
    // convert from [cm] to [m] of dbh by dividing through "10000": d^2*h = (d[cm]/100)^2*h = 1/10000 * d^2*h
    mVolumeFactor = mFormFactor * M_PI_4 / 10000.;

    if (mFoliage_a*mFoliage_b*mRoot_a*mRoot_b*mWoody_a*mWoody_b*mBranch_a*mBranch_b*mWoodDensity*mFormFactor*mSpecificLeafArea == 0.) {
        throw IException( QString("Error setting up species %1: one value is NULL in database.").arg(id()));
    }
}

const double Species::biomassFoliage(const double dbh) const
{
    return mFoliage_a * pow(dbh, mFoliage_b);
}

const double Species::biomassWoody(const double dbh) const
{
    return mWoody_a * pow(dbh, mWoody_b);
}

const double Species::biomassRoot(const double dbh) const
{
    return mRoot_a * pow(dbh, mRoot_b);
}

/** calculate fraction of stem wood increment base on dbh.
    allometric equation: a*d^b -> first derivation: a*b*d^(b-1)
    the ratio for stem is 1 minus the ratio of twigs to total woody increment at current "dbh". */
const double Species::allometricFractionStem(const double dbh) const
{
    double fraction_stem = 1. - (mBranch_a*mBranch_b*pow(dbh, mBranch_b-1.)) / (mWoody_a*mWoody_b*pow(dbh, mWoody_b-1));
    return fraction_stem;
}



