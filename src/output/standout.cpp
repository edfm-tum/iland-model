/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "standout.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"


StandOut::StandOut()
{
    setName("Stand by species/RU", "stand");
    setDescription("Output of aggregates on the level of RU x species. Values are always aggregated per hectare. "\
                   "The output is created after the growth of the year, " \
                   "i.e. output with year=2000 means effectively the state of at the end of the " \
                   "year 2000. The initial state (without any growth) is indicated by the year 'startyear-1'. " \
                   "You can use the 'condition' to control if the output should be created for the current year(see dynamic stand output)");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id() << OutputColumn::species()
               //<< OutputColumn("x_m", "x-coord", OutInteger) <<  OutputColumn("y_m", "y-coord", OutInteger) // temp
              << OutputColumn("count_ha", "tree count (living, >4m height) per ha", OutInteger)
              << OutputColumn("dbh_avg_cm", "average dbh (cm)", OutDouble)
              << OutputColumn("height_avg_m", "average tree height (m)", OutDouble)
              << OutputColumn("volume_m3", "volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("total_carbon_kg", "total carbon in living biomass (aboveground compartments and roots) of all living trees (including regeneration layer) (kg/ha)", OutDouble)
              << OutputColumn("gwl_m3", "'gesamtwuchsleistung' (total growth including removed/dead trees) volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("basal_area_m2", "total basal area at breast height (m2)", OutDouble)
              << OutputColumn("NPP_kg", "sum of NPP (aboveground + belowground) kg Biomass/ha", OutDouble)
              << OutputColumn("NPPabove_kg", "sum of NPP (abovegroundground) kg Biomass/ha", OutDouble)
              << OutputColumn("LAI", "Leafareaindex (m2/m2)", OutDouble)
              << OutputColumn("cohort_count_ha", "number of cohorts in the regeneration layer (<4m) /ha", OutInteger);

 }

void StandOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);
}

void StandOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (!mCondition.isEmpty())
        if (!mCondition.calculate(GlobalSettings::instance()->currentYear()))
            return;


    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area
        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus->constStatistics();
            if (stat.count()==0 && stat.cohortCount()==0)
                continue;
            *this << currentYear() << ru->index() << ru->id() << rus->species()->id(); // keys
            // *this << ru->boundingBox().center().x() << ru->boundingBox().center().y();  // temp
            *this << stat.count() << stat.dbh_avg() << stat.height_avg()
                    << stat.volume() << stat.totalCarbon() << stat.gwl() << stat.basalArea()
                    << stat.npp() << stat.nppAbove() << stat.leafAreaIndex() << stat.cohortCount();
            writeRow();
        }
    }
}
