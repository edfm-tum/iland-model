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

#include "landscapeout.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"

LandscapeOut::LandscapeOut()
{
    setName("Landscape aggregates per species", "landscape");
    setDescription("Output of aggregates on the level of landscape x species. Values are always aggregated per hectare. "\
                   "The output is created after the growth of the year, " \
                   "i.e. output with year=2000 means effectively the state of at the end of the " \
                   "year 2000. The initial state (without any growth) is indicated by the year 'startyear-1'." \
                   "You can use the 'condition' to control if the output should be created for the current year(see also dynamic stand output)");
    columns() << OutputColumn::year() << OutputColumn::species()
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

void LandscapeOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);
}

void LandscapeOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (!mCondition.isEmpty())
        if (!mCondition.calculate(GlobalSettings::instance()->currentYear()))
            return;

    // clear landscape stats
    for (QMap<QString, StandStatistics>::iterator i=mLandscapeStats.begin(); i!= mLandscapeStats.end();++i)
        i.value().clear();

    // extract total stockable area
    double total_area = 0.;
    foreach(const ResourceUnit *ru, m->ruList())
        total_area += ru->stockableArea();

    if (total_area==0.)
        return;

    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area
        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            const StandStatistics &stat = rus->constStatistics();
            if (stat.count()==0. && stat.cohortCount()==0 && stat.gwl()==0.) {
                continue;
            }
            mLandscapeStats[rus->species()->id()].addAreaWeighted(stat, ru->stockableArea() / total_area);
        }
    }
    // now add to output stream
    QMap<QString, StandStatistics>::const_iterator i = mLandscapeStats.constBegin();
    while (i != mLandscapeStats.constEnd()) {
        const StandStatistics &stat = i.value();
        *this << currentYear() << i.key(); // keys: year, species
        *this << stat.count() << stat.dbh_avg() << stat.height_avg()
                              << stat.volume() << stat.totalCarbon() << stat.gwl() << stat.basalArea()
                              << stat.npp() << stat.nppAbove() << stat.leafAreaIndex() << stat.cohortCount();
        writeRow();
        ++i;
    }
}

LandscapeRemovedOut::LandscapeRemovedOut()
{
    mIncludeDeadTrees = false;
    mIncludeHarvestTrees = true;

    setName("Aggregates of removed trees due to death, harvest, and disturbances per species", "landscape_removed");
    setDescription("Aggregates of all removed trees due to 'natural' death, harvest, or disturbance per species and reason. All values are totals for the whole landscape."\
                   "The user can select with options whether to include 'natural' death and harvested trees (which may slow down the processing). " \
                   "Set the setting in the XML project file 'includeNatural' to 'true' to include trees that died due to natural mortality, " \
                   "the setting 'includeHarvest' controls whether to include ('true') or exclude ('false') harvested trees. ");
    columns() << OutputColumn::year()
              << OutputColumn::species()
              << OutputColumn("reason", "Resaon for tree death: 'N': Natural mortality, 'H': Harvest, 'D': Disturbance, 'S': Salvage harvesting, 'C': killed/cut down by management", OutString)
              << OutputColumn("count_ha", "number of died trees (living, >4m height) ", OutInteger)
              << OutputColumn("volume_m3", "sum of volume (geomery, taper factor) in m3", OutDouble)
              << OutputColumn("basal_area_m2", "total basal area at breast height (m2)", OutDouble);

}

void LandscapeRemovedOut::execRemovedTree(const Tree *t, int reason)
{
    Tree::TreeRemovalType rem_type = static_cast<Tree::TreeRemovalType>(reason);
    if (rem_type==Tree::TreeDeath && !mIncludeDeadTrees)
        return;
    if ((rem_type==Tree::TreeHarvest || rem_type==Tree::TreeSalavaged || rem_type==Tree::TreeCutDown) && !mIncludeHarvestTrees)
        return;

    int key = reason*10000 + t->species()->index();
    LROdata &d = mLandscapeRemoval[key];
    d.basal_area += t->basalArea();
    d.volume += t->volume();
    d.n++;


}

void LandscapeRemovedOut::exec()
{
    QHash<int,LROdata>::iterator i  = mLandscapeRemoval.begin();

    while (i != mLandscapeRemoval.end()) {
        if (i.value().n>0) {
            Tree::TreeRemovalType rem_type = static_cast<Tree::TreeRemovalType>(i.key() / 10000);
            int species_index = i.key() % 10000;
            *this << currentYear() << GlobalSettings::instance()->model()->speciesSet()->species(species_index)->id();
            if (rem_type==Tree::TreeDeath) *this << QStringLiteral("N");
            if (rem_type==Tree::TreeHarvest) *this << QStringLiteral("H");
            if (rem_type==Tree::TreeDisturbance) *this << QStringLiteral("D");
            if (rem_type==Tree::TreeSalavaged) *this << QStringLiteral("S");
            if (rem_type==Tree::TreeCutDown) *this << QStringLiteral("C");
            *this << i.value().n << i.value().volume<< i.value().basal_area;
            writeRow();
        }
        ++i;
    }

    // clear data (no need to clear the hash table, right?)
    i = mLandscapeRemoval.begin();
    while (i != mLandscapeRemoval.end()) {
        i.value().clear();
        ++i;
    }
}

void LandscapeRemovedOut::setup()
{
    mIncludeHarvestTrees = settings().valueBool(".includeHarvest", true);
    mIncludeDeadTrees = settings().valueBool(".includeNatural", false);
    Tree::setLandscapeRemovalOutput(this);

}
