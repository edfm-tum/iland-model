#include "understoryout.h"

#include "model.h"
#include "resourceunit.h"
#include "understory.h"
#include "understoryplant.h"

UnderstoryOut::UnderstoryOut()
{
    setName("Understory cover per RU/landscape", "understory");
    setDescription("Understory cover by resource unit / year and/or by landsacpe/year. "\
                   "On resource unit level, the output provides RU proportions, i.e. proportion of stockable area e.g. covered (0..1). \n " \
                   "For landscape level outputs, all variables are aggregated over the landscape and scaled to per hectare stockable area. "\
                   "The area column contains the stockable area (per resource unit / landscape) and can be used to scale to values to the actual value on the ground. \n " \
                   "You can use the 'condition' to control if the output should be created for the current year(see also dynamic stand output).\n" \
                   "The 'conditionRU' can be used to suppress resource-unit-level details; eg. specifying 'in(year,100,200,300)' limits output on reosurce unit level to the years 100,200,300 " \
                   "(leaving 'conditionRU' blank enables details per default).");

    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("area_ha", "total stockable area of the resource unit (ha)", OutDouble)
              << OutputColumn("pft", "Name of the plant functional type", OutString)
              << OutputColumn("area", "total area where the PFT is present (based on 2m cells) (% of stockable area)", OutDouble)
              << OutputColumn("cover", "total area covered by PFT (area x state-specific cover) (% of stockable area)", OutDouble)
              << OutputColumn("areaGain", "total area where PFT regenerated (based on 2m cells, % of stockable area)", OutDouble)
              << OutputColumn("areaLoss", "total area with PFT mortality (based on 2m cells, % of stockable area)", OutDouble)
              << OutputColumn("LAI", "leaf area index of PFT (m2/m2)", OutDouble)
              << OutputColumn("biomass", "biomass of PFT (kg/ha)", OutDouble); // TODO: Clarify biomass units

}

void UnderstoryOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (!m->understory())
        return;

    // global condition
    if (!mCondition.isEmpty() && mCondition.calculate(GlobalSettings::instance()->currentYear())==0.)
        return;

    bool ru_level = true;
    // switch off details if this is indicated in the conditionRU option
    if (!mConditionDetails.isEmpty() && mConditionDetails.calculate(GlobalSettings::instance()->currentYear())==0.)
        ru_level = false;

    QVector<UnderstoryStats> lscp_ru_stats;
    lscp_ru_stats.resize(m->understory()->PFTs().size());

    double total_area = 0;

    for (auto *ru : m->ruList()) {
        auto *us_ru = m->understory()->understoryRU(ru->boundingBox().center());

        double area_factor = ru->stockableArea() / cRUArea; // conversion factor from real area to per ha values
        total_area += area_factor;

        int pft_index = 0;
        for (const auto &stat : us_ru->pftStats()) {
            if (stat.stats.cellsOccupied > 0)  {
                if (ru_level) {
                    const auto *pft = m->understory()->pft(pft_index);
                    *this << currentYear() << ru->index() << ru->id() << area_factor; // keys
                    *this << pft->name();
                    *this << stat.stats.cellsOccupied << stat.stats.cover;
                    *this << stat.established << stat.died;
                    *this << stat.stats.LAI << stat.stats.biomass;
                    writeRow();
                }
                lscp_ru_stats[pft_index].stats += stat.stats;
                lscp_ru_stats[pft_index].established += stat.established * area_factor;
                lscp_ru_stats[pft_index].died += stat.died * area_factor;
            }
            ++pft_index;
        }
    }

    // write landscape level results
    int pft_index = 0;
    for (const auto &stat : lscp_ru_stats) {

        if (stat.stats.cellsOccupied > 0)  {

                const auto *pft = m->understory()->pft(pft_index);
                *this << currentYear() << -1 << -1 << total_area; // keys
                *this << pft->name();
                *this << stat.stats.cellsOccupied / total_area << stat.stats.cover / total_area;
                *this << stat.established / total_area << stat.died / total_area;
                *this << stat.stats.LAI / total_area << stat.stats.biomass / total_area;
                writeRow();
                ++pft_index;

        }
    }

}

void UnderstoryOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);

    condition = settings().value(".conditionRU", "");
    mConditionDetails.setExpression(condition);

}
