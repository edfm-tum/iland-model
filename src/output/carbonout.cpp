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

#include "carbonout.h"
#include "model.h"
#include "resourceunit.h"
#include "snag.h"
#include "soil.h"

CarbonOut::CarbonOut()
{
    setName("Carbon and nitrogen pools above and belowground per RU/yr", "carbon");
    setDescription("Carbon and nitrogen pools (C and N) per resource unit / year. "\
                   "In the output are aggregated above ground pools (kg/ru) " \
                   "together with below ground pools (kg/ha). \n " \
                   "The area column contains the stockable area and can be used to scale to per unit area values. \n " \
                   "__Note__: the figures for soil pools are per hectare even if the stockable area is below one hectare (scaled to 1ha internally). " \
                   "You can use the 'condition' to control if the output should be created for the current year(see also dynamic stand output)");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("area", "total stockable area of the resource unit (m2)", OutInteger)
              << OutputColumn("stem_c", "Stem carbon kg/ru", OutDouble)
              << OutputColumn("stem_n", "Stem nitrogen kg/ru", OutDouble)
              << OutputColumn("branch_c", "branches carbon kg/ru", OutDouble)
              << OutputColumn("branch_n", "branches nitrogen kg/ru", OutDouble)
              << OutputColumn("foliage_c", "Foliage carbon kg/ru", OutDouble)
              << OutputColumn("foliage_n", "Foliage nitrogen kg/ru", OutDouble)
              << OutputColumn("coarseRoot_c", "coarse root carbon kg/ru", OutDouble)
              << OutputColumn("coarseRoot_n", "coarse root nitrogen kg/ru", OutDouble)
              << OutputColumn("fineRoot_c", "fine root carbon kg/ru", OutDouble)
              << OutputColumn("fineRoot_n", "fine root nitrogen kg/ru", OutDouble)
              << OutputColumn("regeneration_c", "total carbon in regeneration layer (h<4m) kg/ru", OutDouble)
              << OutputColumn("regeneration_n", "total nitrogen in regeneration layer (h<4m) kg/ru", OutDouble)
              << OutputColumn("snags_c", "standing dead wood carbon kg/ru", OutDouble)
              << OutputColumn("snags_n", "standing dead wood nitrogen kg/ru", OutDouble)
              << OutputColumn("snagsOther_c", "branches and coarse roots of standing dead trees, carbon kg/ru", OutDouble)
              << OutputColumn("snagsOther_n", "branches and coarse roots of standing dead trees, nitrogen kg/ru", OutDouble)
              << OutputColumn("downedWood_c", "downed woody debris (yR), carbon kg/ha", OutDouble)
              << OutputColumn("downedWood_n", "downed woody debris (yR), nitrogen kg/ga", OutDouble)
              << OutputColumn("litter_c", "soil litter (yl), carbon kg/ha", OutDouble)
              << OutputColumn("litter_n", "soil litter (yl), nitrogen kg/ha", OutDouble)
              << OutputColumn("soil_c", "soil organic matter (som), carbon kg/ha", OutDouble)
              << OutputColumn("soil_n", "soil organic matter (som), nitrogen kg/ha", OutDouble);


}

void CarbonOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);
}


void CarbonOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (!mCondition.isEmpty())
        if (!mCondition.calculate(GlobalSettings::instance()->currentYear()))
            return;

    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1 || !ru->snag())
            continue; // do not include if out of project area
        *this << currentYear() << ru->index() << ru->id() << ru->stockableArea(); // keys
        // biomass from trees
        const StandStatistics &s = ru->statistics();
        *this << s.cStem() << s.nStem()   // stem
              << s.cBranch() << s.nBranch()   // branch
              << s.cFoliage() << s.nFoliage()   // foliage
              << s.cCoarseRoot() << s.nCoarseRoot()   // coarse roots
              << s.cFineRoot() << s.nFineRoot();   // fine roots

        // biomass from regeneration
        *this << s.cRegeneration() << s.nRegeneration();

        // biomass from standing dead wood
        *this << ru->snag()->totalSWD().C << ru->snag()->totalSWD().N   // snags
              << ru->snag()->totalOtherWood().C << ru->snag()->totalOtherWood().N;   // snags, other (branch + coarse root)

        // biomass from soil (convert from t/ha -> kg/ha)
        *this << ru->soil()->youngRefractory().C*1000. << ru->soil()->youngRefractory().N*1000.   // wood
              << ru->soil()->youngLabile().C*1000. << ru->soil()->youngLabile().N*1000.   // litter
              << ru->soil()->oldOrganicMatter().C*1000. << ru->soil()->oldOrganicMatter().N*1000.;   // soil

        writeRow();
    }

}

