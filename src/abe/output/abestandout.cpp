#include "abestandout.h"
#include "globalsettings.h"

#include "forestmanagementengine.h"
#include "fmstand.h"
#include "fmunit.h"
#include "scheduler.h"

namespace ABE {

ABEStandOut::ABEStandOut()
{
    setName("Annual harvests on stand level.", "abeStand");
    setDescription("This output provides details about realized timber harvests on stand level. " \
                   "The timber is provided as standing timber per hectare. The total harvest on the stand is the sum of thinning, final, and disturbed volume.");
    columns() << OutputColumn::year()
              << OutputColumn("unitid", "unique identifier of the planning unit", OutString)
              << OutputColumn("standid", "unique identifier of the forest stand", OutInteger)
              << OutputColumn("area", "total area of the forest stand (ha)", OutDouble)
              << OutputColumn("activity", "name of the management activity that is executed", OutString)
              << OutputColumn("volumeAfter", "standing timber volume after the harvest operation (m3/ha)", OutDouble)
              << OutputColumn("volumeThinning", "removed timber volume due to thinning, m3/ha", OutDouble)
              << OutputColumn("volumeFinal", "removed timber volume due to final harvests (regeneration cuts) and due to salvage operations, m3/ha", OutDouble)
              << OutputColumn("volumeDisturbed", "disturbed trees on the stand, m3/ha. Note: all killed trees are recorded here, even if not 100% of those trees are salvaged (due to size constraints)", OutDouble);
}

void ABEStandOut::exec()
{
    foreach(const FMStand *stand, ForestManagementEngine::instance()->stands()) {
        if (stand->totalHarvest()>0.) {
            *this << currentYear();
            *this << stand->unit()->id() << stand->id() << stand->area();
            *this << (stand->lastExecutedActivity()?stand->lastExecutedActivity()->name():QString());
            *this << qRound(stand->volume()*100.)/100. << stand->totalThinningHarvest() / stand->area() //  thinning alone
                                     << (stand->totalHarvest() - stand->totalThinningHarvest() ) / stand->area() // final harvests (including salvage operations)
                                     << stand->disturbedTimber() / stand->area();  // disturbed trees on the stand

            writeRow();
        }
    }
}

void ABEStandOut::setup()
{

}




} // namespace
