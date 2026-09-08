/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

#include "deadtreeout.h"
#include "debugtimer.h"
#include "model.h"
#include "resourceunit.h"
#include "snag.h"
#include "species.h"
#include "expressionwrapper.h"

DeadTreeOut::DeadTreeOut()
{
    setName("Dead Tree Output", "deadtree");
    setDescription("Output of individual standing and lying dead trees. Use the ''filter'' property to reduce amount of data (filter by dead tree variables such as dbh, species, snag, ruindex).\n" \
                   "The 'condition' property can be used to control output timing (variable 'year').\n" \
                   "The output is triggered at the end of the year and after initialization (year 0).\n\n" \
                   "The column 'type' distinguishes standing snags ('S') and downed dead wood ('D').\n\n" \
                   "The classification of 'decayClass' (1 to 5) is based on the fraction of carbon lost (remaining biomass relative to biomass at death), with threshold values defined in 'model.settings.soil.decayClassThresholds' (see https://iland-model.org/dead-trees).\n\n" \
                   "The column 'reason' indicates the cause of death:\n\n" \
                   "||__reason__|__description__\n" \
                   "mort|natural mortality / competition\n" \
                   "bb|bark beetle\n" \
                   "wind|wind disturbance\n" \
                   "fire|fire disturbance\n" \
                   "mgmt|management / cutdown||");

    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id() << OutputColumn::species()
              << OutputColumn("x", "position of the dead tree, x-direction (m)", OutDouble)
              << OutputColumn("y", "position of the dead tree, y-direction (m)", OutDouble)
              << OutputColumn("type", "type of dead wood: 'S' (standing snag), 'D' (downed dead wood)", OutString)
              << OutputColumn("dbh", "dbh (cm) of the tree at time of death", OutDouble)
              << OutputColumn("volume_m3", "volume of stem at time of death (m3)", OutDouble)
              << OutputColumn("decayClass", "decay class (1..5), based on carbon loss defined in 'model.settings.soil.decayClassThresholds' (see https://iland-model.org/dead-trees)", OutInteger)
              << OutputColumn("biomass_kg", "current biomass of the dead tree (kg)", OutDouble)
              << OutputColumn("remaining", "proportion of remaining biomass relative to biomass at death (0..1)", OutDouble)
              << OutputColumn("yearsStanding", "years since death standing as snag", OutInteger)
              << OutputColumn("yearsDowned", "years since downed on the ground", OutInteger)
              << OutputColumn("reason", "reason of death ('mort', 'bb', 'wind', 'fire', 'mgmt')", OutString);
}

void DeadTreeOut::setup()
{

    QString filter = settings().value(".filter", "");
    mFilter.setExpression(filter);

    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);
}

void DeadTreeOut::exec()
{
    if (!mCondition.isEmpty()) {
        if (!mCondition.calculate(GlobalSettings::instance()->currentYear()))
            return;
    }

    Model *m = GlobalSettings::instance()->model();
    if (!m)
        return;
    if (!m->settings().carbonCycleEnabled)
        return;

    DebugTimer t("DeadTreeOut::exec()");
    DeadTreeWrapper dw;
    mFilter.setModelObject(&dw);

    for (const auto &ru : m->ruList()) {
        if (!ru || ru->id() == -1 || !ru->snag())
            continue;

        for (const auto &dt : ru->snag()->deadTrees()) {
            if (!mFilter.isEmpty()) {
                dw.setDeadTree(&dt, ru);
                if (!mFilter.executeBool())
                    continue;
            }

            QString reasonStr;
            switch (dt.reason()) {
                case 1: reasonStr = "mort"; break;
                case 2: reasonStr = "bb"; break;
                case 3: reasonStr = "wind"; break;
                case 4: reasonStr = "fire"; break;
                case 5: reasonStr = "mgmt"; break;
                default: reasonStr = "unknown"; break;
            }

            QString typeStr = dt.isStanding() ? "S" : "D";

            *this << currentYear() << ru->index() << ru->id() << (dt.species() ? dt.species()->id() : QString());
            *this << dt.x() << dt.y() << typeStr << dt.dbh()
                  << dt.volume() << dt.decayClass() << dt.biomass() << dt.proportionBiomass()
                  << dt.yearsStanding() << dt.yearsDowned() << reasonStr;
            writeRow();
        }
    }
}
