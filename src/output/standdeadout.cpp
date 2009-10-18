#include "standdeadout.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"


StandDeadOut::StandDeadOut()
{
    setName("Dead trees by species/RU", "standdead");
    setDescription("Died trees in current year on the level of RU x species. The output is created after the growth of the year, " \
                   "i.e. the growth of year trees are dying in is included! NPP and NPP_kg are not recorded for trees that " \
                   "are removed during management. ");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species()
              << OutputColumn("count_ha", "tree count (living)", OutInteger)
              << OutputColumn("dbh_avg_cm", "average dbh (cm)", OutDouble)
              << OutputColumn("height_avg_m", "average tree height (m)", OutDouble)
              << OutputColumn("volume_m3", "volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("basal_area_m2", "total basal area at breast height (m2)", OutDouble)
              << OutputColumn("NPP_kg", "sum of NPP (aboveground + belowground) kg Biomass/ha", OutDouble)
              << OutputColumn("NPPabove_kg", "sum of NPP (abovegroundground) kg Biomass/ha", OutDouble);

 }

void StandDeadOut::setup()
{
}

void StandDeadOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    foreach(ResourceUnit *ru, m->ruList()) {
        foreach(const ResourceUnitSpecies &rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus.constStatisticsDead();
            if (stat.count()==0)
                continue;
            *this << currentYear() << ru->index() << rus.species()->id(); // keys
            *this << stat.count() << stat.dbh_avg() << stat.height_avg() << stat.volume() << stat.basalArea()
                    << stat.npp() << stat.nppAbove();
            writeRow();
        }
    }
}
