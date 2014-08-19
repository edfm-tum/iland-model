#include "abestandout.h"
#include "globalsettings.h"

#include "forestmanagementengine.h"
#include "fmstand.h"
#include "fmunit.h"
#include "scheduler.h"

namespace ABE {

ABEStandOut::ABEStandOut()
{
    setName("Annual stand output (state).", "abeStand");
    setDescription("This output provides details about realized timber harvests on stand level. " \
                   "The timber is provided as standing timber per hectare. The total harvest on the stand is the sum of thinning, final, and disturbed volume.");
    columns() << OutputColumn::year()
              << OutputColumn("unitid", "unique identifier of the planning unit", OutString)
              << OutputColumn("standid", "unique identifier of the forest stand", OutInteger)
              << OutputColumn("area", "total area of the forest stand (ha)", OutDouble)
              << OutputColumn("volume", "standing timber volume (after harvests of the year) (m3/ha)", OutDouble)
              << OutputColumn("basalarea", "basal area (trees >4m) (m2/ha)", OutDouble)
              << OutputColumn("dbh", "mean diameter (basal area weighted, of trees >4m) (cm)", OutDouble)
              << OutputColumn("height", "mean stand tree height (basal area weighted, of trees >4m)(cm)", OutDouble)
              << OutputColumn("stems", "number of trees (trees >4m) per ha", OutDouble)
                 ;
}

void ABEStandOut::exec()
{
    foreach(FMStand *stand, ForestManagementEngine::instance()->stands()) {

        // Note: EXPENSIVE reload operation for every stand and every year....
        stand->reload();

        *this << currentYear();
        *this << stand->unit()->id() << stand->id() << stand->area();
        *this << qRound(stand->volume()*100.)/100.;
        *this << qRound(stand->basalArea()*100.)/100.;
        *this << qRound(stand->dbh()*100.)/100.;
        *this << qRound(stand->height()*100.)/100.;
        *this << qRound(stand->stems());
        writeRow();

    }
}

void ABEStandOut::setup()
{

}




} // namespace
