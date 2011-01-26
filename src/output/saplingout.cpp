#include "saplingout.h"
#include "model.h"
#include "resourceunit.h"
#include "sapling.h"
#include "species.h"

SaplingOut::SaplingOut()
{

    setName("Sapling Output", "sapling");
    setDescription("Output of the establishment/sapling layer per resource unit and species.\n" \
                   "The output covers trees between a dbh of 1cm and the recruitment threshold (i.e. a height of 4m)." \
                   "Cohorts with a dbh < 1cm are counted in 'cohort_count_ha' but not used for average calculations.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id() << OutputColumn::species()
            << OutputColumn("count_ha", "number of represented individuals per ha.", OutInteger)
            << OutputColumn("cohort_count_ha", "number of cohorts per ha.", OutInteger)
            << OutputColumn("height_avg_m", "arithmetic average height (m) (using represented individuals >1cm dbh)", OutDouble)
            << OutputColumn("dbh_avg_cm", "arithmetic average diameter (cm) (using represented individuals >1cm dbh)", OutDouble)
            << OutputColumn("age_avg", "arithmetic average age of the saplings (years) (using represented individuals >1cm dbh)", OutDouble);
 }

void SaplingOut::setup()
{
}

void SaplingOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    double n, avg_dbh, avg_height, avg_age;
    foreach(ResourceUnit *ru, m->ruList()) {
        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus->constStatistics();
            const Sapling &sap = rus->sapling();

            if (stat.saplingCount()==0)
                continue;
            *this << currentYear() << ru->index() << ru->id() << rus->species()->id(); // keys

            // calculate statistics based on the number of represented trees per cohort
            n = sap.livingStemNumber(avg_dbh, avg_height, avg_age);
            *this << n
                  << stat.saplingCount()
                  << avg_height
                  << avg_dbh
                  << avg_age;
            writeRow();
        }
    }
}
