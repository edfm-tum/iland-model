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
                   "Cohorts with a dbh < 1cm are counted in 'cohort_count_ha' but not used for average calculations.\n\n" \
                   "You can specify a 'condition' to limit execution for specific time/ area with the variables 'ru' (resource unit id) and 'year' (the current year)");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id() << OutputColumn::species()
            << OutputColumn("count_ha", "number of represented individuals per ha.", OutInteger)
            << OutputColumn("cohort_count_ha", "number of cohorts per ha.", OutInteger)
            << OutputColumn("height_avg_m", "arithmetic average height (m) (using represented individuals >1cm dbh)", OutDouble)
            << OutputColumn("dbh_avg_cm", "arithmetic average diameter (cm) (using represented individuals >1cm dbh)", OutDouble)
            << OutputColumn("age_avg", "arithmetic average age of the saplings (years) (using represented individuals >1cm dbh)", OutDouble);
 }

void SaplingOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);
    if (!mCondition.isEmpty()) {
        mVarRu = mCondition.addVar("ru");
        mVarYear = mCondition.addVar("year");
    }
}

void SaplingOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    double n, avg_dbh, avg_height, avg_age;
    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area

        if (!mCondition.isEmpty()) {
            *mVarRu = ru->id();
            *mVarYear = GlobalSettings::instance()->currentYear();
            if (!mCondition.execute() )
                continue;
        }

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
