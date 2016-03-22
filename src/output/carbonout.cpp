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
    setDescription("Carbon and nitrogen pools (C and N) per resource unit / year and/or by landsacpe/year. "\
                   "On resource unit level, the outputs contain aggregated above ground pools (kg/ru) " \
                   "and below ground pools (kg/ha). \n " \
                   "For landscape level outputs, all variables are scaled to kg/ ha stockable area, e.g., landscape-sum of stem_c [kg] is stem_c[kg/ha] * area[m2] / 10000=kg "\
                   "(equal for aboveground and soil pools)."\
                   "The area column contains the stockable area (per resource unit / landscape) and can be used to scale to per unit area values. \n " \
                   "__Note__: the figures for soil pools are per hectare even if the stockable area is below one hectare (scaled to 1ha internally). " \
                   "You can use the 'condition' to control if the output should be created for the current year(see also dynamic stand output).\n" \
                   "The 'conditionRU' can be used to suppress resource-unit-level details; eg. specifying 'in(year,100,200,300)' limits output on reosurce unit level to the years 100,200,300 " \
                   "(leaving 'conditionRU' blank enables details per default).");
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

    condition = settings().value(".conditionRU", "");
    mConditionDetails.setExpression(condition);

}


void CarbonOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    // global condition
    if (!mCondition.isEmpty() && mCondition.calculate(GlobalSettings::instance()->currentYear())==0.)
        return;

    bool ru_level = true;
    // switch off details if this is indicated in the conditionRU option
    if (!mConditionDetails.isEmpty() && mConditionDetails.calculate(GlobalSettings::instance()->currentYear())==0.)
        ru_level = false;


    QVector<double> v(23, 0.); // 8 data values
    QVector<double>::iterator vit;

    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1 || !ru->snag())
            continue; // do not include if out of project area

        const StandStatistics &s = ru->statistics();
        int ru_count = 0;
        if (ru_level) {
            *this << currentYear() << ru->index() << ru->id() << ru->stockableArea(); // keys
            // biomass from trees

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
        // landscape level statistics
        double rusa = ru->stockableArea() / (cRUSize*cRUSize); // stockable area ha / ha [0..1]
        ++ru_count;
        vit = v.begin();
        *vit++ += ru->stockableArea();
        // carbon pools aboveground are in kg/resource unit, e.g., the sum of stem-carbon of all trees, so no scaling required
        *vit++ += s.cStem(); *vit++ += s.nStem();
        *vit++ += s.cBranch(); *vit++ += s.nBranch();
        *vit++ += s.cFoliage(); *vit++ += s.nFoliage();
        *vit++ += s.cCoarseRoot(); *vit++ += s.nCoarseRoot();
        *vit++ += s.cFineRoot(); *vit++ += s.nFineRoot();
        // regen
        *vit++ += s.cRegeneration(); *vit++ += s.nRegeneration();
        // standing dead wood
        *vit++ += ru->snag()->totalSWD().C; *vit++ += ru->snag()->totalSWD().N;
        *vit++ += ru->snag()->totalOtherWood().C; *vit++ += ru->snag()->totalOtherWood().N;
        // biomass from soil (converstion to kg/ha), and scale with fraction of stockable area
        *vit++ += ru->soil()->youngRefractory().C*rusa * 1000.; *vit++ += ru->soil()->youngRefractory().N*rusa * 1000.;
        *vit++ += ru->soil()->youngLabile().C*rusa * 1000.; *vit++ += ru->soil()->youngLabile().N*rusa * 1000.;
        *vit++ += ru->soil()->oldOrganicMatter().C*rusa * 1000.; *vit++ += ru->soil()->oldOrganicMatter().N*rusa * 1000.;

    }
    // write landscape sums
    double total_stockable_area = v[0]/ (cRUSize*cRUSize); // convert to ha of stockable area
    *this << currentYear() << -1 << -1; // keys
    *this << v[0]; // stockable area [m2]
    for (int i=1;i<v.size();++i)
        *this << v[i] / total_stockable_area;
    writeRow();

}

