#include "unitout.h"
#include "globalsettings.h"

#include "forestmanagementengine.h"
#include "fmunit.h"
#include "scheduler.h"

namespace ABE {

UnitOut::UnitOut()
{
    setName("Annual harvests and harvest plan on unit level.", "abeUnit");
    setDescription("Carbon fluxes per resource unit and year. Note that all fluxes are reported on a per ru basis, " \
                   "i.e. on the actual simulated area. Thus summing over all ru should give the overall C fluxes for"\
                   " the simulated landscape. Fluxes that are internally calculated on a per ha basis thus need to be "\
                   "scaled to the stockable area. Furthermore, the following sign convention is used in iLand: fluxes "\
                   "from the atmosphere to the ecosystem are positive, while C leaving the ecosystem is reported as negative C flux.");
    columns() << OutputColumn::year()
              << OutputColumn("id", "unique identifier of the planning unit", OutString)
              << OutputColumn("area", "total area of the unit (ha)", OutDouble)
              << OutputColumn("age", "mean stand age (area weighted) (updated every 10yrs)", OutDouble)
              << OutputColumn("volume", "mean standing volume (updated every 10yrs), m3/ha", OutDouble)
              << OutputColumn("MAI", "mean annual increment (updated every 10yrs), m3/ha*yr", OutDouble)
              << OutputColumn("decadePlan", "planned mean harvest per year for the decade (m3/ha*yr)", OutDouble)
              << OutputColumn("annualPlan", "updated annual plan for the year, m3/ha*yr", OutDouble)
              << OutputColumn("realizedHarvest", "total harvested timber volume, m3/ha*yr", OutDouble)
              << OutputColumn("finalHarvest", "total harvested timber of planned final harvests, m3/ha*yr", OutDouble)
              << OutputColumn("thinningHarvest", "total harvested timber due to tending and thinning operations, m3/ha*yr", OutDouble)
              << OutputColumn("salvageHarvest", "total harvested timber due to salvage operations, m3/ha*yr", OutDouble);


}


void UnitOut::exec()
{
    FMUnit *unit;
    double salvage_harvest, annual_target;
    foreach(unit, ForestManagementEngine::instance()->mUnits) {
        *this << currentYear() << unit->id(); // keys
        *this << unit->area();
        *this << unit->mMeanAge << unit->mTotalVolume/unit->area() << unit->mMAI;
        *this << unit->mAnnualHarvestTarget;
        if (unit->scheduler()) {
            salvage_harvest = unit->scheduler()->mExtraHarvest;
            annual_target = unit->scheduler()->mHarvestTarget;
        } else {
            salvage_harvest = 0.;
            annual_target = 0.;
        }
        *this << annual_target << unit->annualHarvest()/unit->area() << 0. << 0. << salvage_harvest;

        writeRow();
     }
}

void ABE::UnitOut::setup()
{

}


} // namespace
