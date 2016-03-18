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
#include "abestandremovalout.h"
#include "globalsettings.h"

#include "forestmanagementengine.h"
#include "fmstand.h"
#include "fmunit.h"
#include "scheduler.h"

namespace ABE {

ABEStandRemovalOut::ABEStandRemovalOut()
{
    setName("Annual harvests on stand level.", "abeStandRemoval");
    setDescription("This output provides details about realized timber harvests on stand level. " \
                   "The timber is provided as standing timber per hectare. The total harvest on the stand is the sum of thinning and final.");
    columns() << OutputColumn::year()
              << OutputColumn("unitid", "unique identifier of the planning unit", OutString)
              << OutputColumn("standid", "unique identifier of the forest stand", OutInteger)
              << OutputColumn("area", "total area of the forest stand (ha)", OutDouble)
              << OutputColumn("age", "absolute stand age at the time of the activity (yrs)", OutDouble)
              << OutputColumn("activity", "name of the management activity that is executed", OutString)
              << OutputColumn("volumeAfter", "standing timber volume after the harvest operation (m3/ha)", OutDouble)
              << OutputColumn("volumeThinning", "removed timber volume due to thinning, m3/ha", OutDouble)
              << OutputColumn("volumeFinal", "removed timber volume due to final harvests (regeneration cuts) and due to salvage operations, m3/ha", OutDouble)
              << OutputColumn("volumeDisturbed", "disturbed trees on the stand, m3/ha. Note: all killed trees are recorded here,also those trees that are not salvaged (due to size and other constraints)", OutDouble);
}

void ABEStandRemovalOut::exec()
{
    foreach(const FMStand *stand, ForestManagementEngine::instance()->stands()) {
        if (stand->totalHarvest()>0.) {
            *this << currentYear();
            *this << stand->unit()->id() << stand->id() << stand->area() << stand->lastExecutionAge();
            *this << (stand->lastExecutedActivity()?stand->lastExecutedActivity()->name():QString());
            *this << qRound(stand->volume()*100.)/100. << stand->totalThinningHarvest() / stand->area() //  thinning alone
                                     << (stand->totalHarvest() - stand->totalThinningHarvest() ) / stand->area() // final harvests (including salvage operations)
                                     << stand->disturbedTimber() / stand->area();  // disturbed trees on the stand

            writeRow();
        }
    }
}

void ABEStandRemovalOut::setup()
{

}




} // namespace
