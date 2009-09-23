#include "standout.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"


StandOut::StandOut()
{
    setName("Stand by species/RU", "stand");
    setDescription("Output of aggregates on the level of RU x species.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species()
              << OutputColumn("count_ha", "tree count (living)", OutInteger)
              << OutputColumn("dbh_avg_cm", "average dbh (cm)", OutDouble)
              << OutputColumn("height_avg_m", "average tree height (m)", OutDouble)
              << OutputColumn("volume_m3", "volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("basal_area_m2", "total basal area at breast height (m2)", OutDouble);
 }

void StandOut::setup()
{
}

void StandOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    foreach(ResourceUnit *ru, m->ruList()) {
        foreach(const ResourceUnitSpecies &rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus.constStatistics();
            if (stat.count()==0)
                continue;
            *this << currentYear() << ru->index() << rus.species()->id(); // keys
            *this << stat.count() << stat.dbh_avg() << stat.height_avg() << stat.volume() << stat.basalArea();
            writeRow();
        }
    }
}
