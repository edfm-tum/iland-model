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
    mStamp.load( GlobalSettings::instance()->path(stampFile, "lip") );
    // attach writer stamps to reader stamps
    mStamp.attachReaderStamps(mSet->readerStamps());

    // setup allometries
    mBiomassLeaf.setAndParse(stringVar("bmLeaf"));
    mBiomassStem.setAndParse(stringVar("bmStem"));
    mBiomassRoot.setAndParse(stringVar("bmRoot"));

    mSpecificLeafArea = doubleVar("specificLeafArea");

    // turnover rates
    mTurnoverLeaf = doubleVar("turnoverLeaf");
    mTurnoverStem = doubleVar("turnoverStem");
    mTurnoverRoot = doubleVar("turnoverRoot");

    // hd-relations
    mHDlow.setAndParse(stringVar("HDlow"));
    mHDhigh.setAndParse(stringVar("HDhigh"));

    // form/density
    mWoodDensity = doubleVar("woodDensity");
    mFormFactor = doubleVar("formFactor");
    // volume = density*formfactor*pi/4 *d^2*h -> volume = volumefactor * d^2 * h
    // convert from [cm] to [m] of dbh by dividing through "10000": d^2*h = (d[cm]/100)^2*h = 1/10000 * d^2*h
    mVolumeFactor = mWoodDensity * mFormFactor * M_PI_4 / 10000.;

    qDebug() << "biomass leaf. 10:->" << mBiomassLeaf.calculate(10.);

}



