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

#include "managementout.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"

ManagementOut::ManagementOut()
{
    setName("Removed trees by species/RU", "management");
    setDescription("Aggregates for trees that are removed in current year on the level of RU x species. All values are scaled to one hectare."\
                   "The output is created after the growth of the year, " \
                   "i.e. the growth of the year in which trees are dying, is included!  " \
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
