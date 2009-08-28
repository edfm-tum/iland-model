#include <QtCore>
#include "species.h"
#include "speciesset.h"
#include "stampcontainer.h"



const Stamp* Species::stamp(const float dbh, const float height) const
{
    Q_ASSERT_X(m_stamps!=0, "Species::stamp", "stamp NULL");
    return m_stamps->stamp(dbh, height);
}

/** main setup routine for tree species.
  Data is fetched from the open query (or file, ...) in the parent SpeciesSet using xyzVar() functions.
  This is called
*/
void Species::setup()
{
    Q_ASSERT(mSet == 0);
    // setup general information
    mId = stringVar("shortName");
    mName = stringVar("name");
    QString stampFile = stringVar("LIPFile"); // todo: process!!

    // setup allometries
    mBiomassLeaf.setExpression(stringVar("bmLeaf"));
    mBiomassStem.setExpression(stringVar("bmStem"));
    mBiomassRoot.setExpression(stringVar("bmRoot"));

}



