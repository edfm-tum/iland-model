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

    qDebug() << "biomass leaf. 10:->" << mBiomassLeaf.calculate(10.);

}



