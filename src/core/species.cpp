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
    mBiomassLeaf.setExpression(stringVar("bmLeaf"));
    mBiomassStem.setExpression(stringVar("bmStem"));
    mBiomassRoot.setExpression(stringVar("bmRoot"));

    mSpecificLeafArea = doubleVar("specificLeafArea");

    // turnover rates
    mTurnoverLeaf = doubleVar("turnoverLeaf");
    mTurnoverStem = doubleVar("turnoverStem");
    mTurnoverRoot = doubleVar("turnoverRoot");

    // hd-relations
    mHDlow.setExpression(stringVar("HDlow"));
    mHDhigh.setExpression(stringVar("HDhigh"));

    qDebug() << "biomass leaf. 10:->" << mBiomassLeaf.calculate(10.);

}



