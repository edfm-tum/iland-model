#include "managementout.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"

ManagementOut::ManagementOut()
{
    setName("Removed trees by species/RU", "management");
    setDescription("Aggregates for trees that are removed in current year on the level of RU x species. "\
                   "The output is created after the growth of the year, " \
                   "i.e. the growth of the year trees are dying in is included!  " \
                   " ");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id() << OutputColumn::species()
              << OutputColumn("count_ha", "tree count (living)", OutInteger)
              << OutputColumn("dbh_avg_cm", "average dbh (cm)", OutDouble)
              << OutputColumn("height_avg_m", "average tree height (m)", OutDouble)
              << OutputColumn("volume_m3", "volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("basal_area_m2", "total basal area at breast height (m2)", OutDouble);


 }

void ManagementOut::setup()
{
}

void ManagementOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area

        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus->constStatisticsMgmt();
            if (stat.count()==0)
                continue;
            *this << currentYear() << ru->index() << ru->id() << rus->species()->id(); // keys
            *this << stat.count() << stat.dbh_avg() << stat.height_avg() << stat.volume() << stat.basalArea();

            writeRow();
        }
    }
}
