#include "unitout.h"
#include "globalsettings.h"

#include "forestmanagementengine.h"
#include "fmunit.h"
#include "scheduler.h"

namespace ABE {

UnitOut::UnitOut()
{
    setName("Annual harvests and harvest plan on unit level.", "abeUnit");
    setDescription("The output provides planned and realized harvests on the level of planning units. " \
                   "Note that the planning unit area, mean age, mean volume and MAI are only updated every 10 years. "\
                   "Harvested timber is given as 'realizedHarvest', which is the sum of 'finalHarvest' and 'thinningHarvest.' "\
                   "The 'salvageHarvest' is provided extra, but already accounted for in the 'finalHarvest' column");
    columns() << OutputColumn::year()
              << OutputColumn("id", "unique identifier of the planning unit", OutString)
              << OutputColumn("area", "total area of the unit (ha)", OutDouble)
              << OutputColumn("age", "mean stand age (area weighted) (updated every 10yrs)", OutDouble)
              << OutputColumn("volume", "mean standing volume (updated every 10yrs), m3/ha", OutDouble)
              << OutputColumn("MAI", "mean annual increment (updated every 10yrs), m3/ha*yr", OutDouble)
              << OutputColumn("decadePlan", "planned mean harvest per year for the decade (m3/ha*yr)", OutDouble)
              << OutputColumn("annualPlan", "updated annual plan for the year, m3/ha*yr", OutDouble)
              << OutputColumn("runningDelta", "current aggregated difference between planned and realied harvests; positive: more realized than planned harvests, m3/ha*yr", OutDouble)
              << OutputColumn("realizedHarvest", "total harvested timber volume, m3/ha*yr", OutDouble)
              << OutputColumn("finalHarvest", "total harvested timber of planned final harvests (including salvage harvests), m3/ha*yr", OutDouble)
              << OutputColumn("thinningHarvest", "total harvested timber due to tending and thinning operations, m3/ha*yr", OutDouble)
              << OutputColumn("salvageHarvest", "total harvested timber due to salvage operations (also included in final harvests), m3/ha*yr", OutDouble);


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
            salvage_harvest = unit->scheduler()->mExtraHarvest / unit->area();
            annual_target = unit->scheduler()->mFinalCutTarget;
        } else {
            salvage_harvest = 0.;
            annual_target = 0.;

        }
        double thin_h = unit->annualThinningHarvest()/unit->area();
        *this << annual_target << unit->mTotalPlanDeviation
                               << unit->annualTotalHarvest()/unit->area() // total realized
                               << unit->annualTotalHarvest()/unit->area() - thin_h  // final
                               << thin_h // thinning
                               << salvage_harvest; // salvaging

        writeRow();
     }
}

void ABE::UnitOut::setup()
{

}


} // namespace
