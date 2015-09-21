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

#include "barkbeetleout.h"
#include "barkbeetlemodule.h"

BarkBeetleOut::BarkBeetleOut()
{
    mBB = 0;
    setName("BarkBeetle module output", "barkbeetle");
    setDescription("Barkbeetle related outputs per year. "\
                   "The outputs are created after each year (or spread event) and contain information about bark beetle generations, spread and damage for the total landscape.\n " \
                   "For spatially explicit outputs, see also the script functions for extracting gridded data.");
    columns() << OutputColumn::year()
              << OutputColumn("initialInfestedArea_ha", "Area of infested pixels (ha) at the start of the iteration (i.e. before winter mortality or background activation happen).", OutDouble)
              << OutputColumn("backgroundMortality_ha", "Area of infested pixels (ha) that die due to winter mortality.", OutDouble)
              << OutputColumn("backgroundActivation_ha", "Area of (not infested) pixels (ha) that are 'ignited' and consequently a source of bark beetles.", OutDouble)
              << OutputColumn("spreadCohorts", "Number of bark beetle 'packages' (x1000) that are spread from the source pixels (kilo-cohorts).", OutDouble)
              << OutputColumn("landedCohorts", "Number of bark beetle 'packages' (x1000) that reach potential hosts (cohorts x 1000).", OutDouble)
              << OutputColumn("landedArea_ha", "Area (ha) of potential host trees where bark beetles landed.", OutDouble)
              << OutputColumn("infestedArea_ha", "Area (ha) of newly infected host pixels.", OutDouble)
              << OutputColumn("killedTrees", "total number of Norway spruce trees  that were killed in this iteration.", OutDouble)
              << OutputColumn("killedBasalArea", "Total Basal Area of killed trees in the current year.", OutDouble);


}


void BarkBeetleOut::exec()
{
    const double area_factor = 0.01; // area in ha of one pixel
    *this << currentYear();
    *this << mBB->stats.infestedStart*area_factor << mBB->stats.NWinterMortality*area_factor << mBB->stats.infestedBackground*area_factor;

    *this << mBB->stats.NCohortsSpread * 0.001 << mBB->stats.NCohortsLanded*0.001 << mBB->stats.NPixelsLanded*area_factor;
    *this << mBB->stats.NInfested*area_factor;
    *this << mBB->stats.NTreesKilled << mBB->stats.BasalAreaKilled;

    writeRow();
}

void BarkBeetleOut::setup()
{

}

