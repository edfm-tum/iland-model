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
              << OutputColumn("fireId", "unique ID of the fire event (1..N) on the whole project area.", OutInteger)
              << OutputColumn("area_plan_m2", "Area of the planned fire m2", OutInteger)
              << OutputColumn("area_m2", "Realized area of burnt cells m2", OutInteger);

//     qDebug() << "iter/background-inf/winter-mort/N spread/N landed/N infested: " << mIteration << stats.infestedBackground << stats.NWinterMortality << stats.NCohortsSpread << stats.NCohortsLanded << stats.NInfested;

}

BarkBeetleOut::~BarkBeetleOut()
{

}

void BarkBeetleOut::exec()
{
    *this << currentYear();
    // ....

    writeRow();
}

void BarkBeetleOut::setup()
{

}

