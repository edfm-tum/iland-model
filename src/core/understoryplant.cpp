#include "understoryplant.h"
#include "understory.h"
#include "saplings.h"
#include "speciesset.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "microclimate.h"
#include "climate.h"
#include "soil.h"

UnderstoryPlant::UnderstoryPlant() {}



bool UnderstoryCell::hasPft(const UnderstoryPFT *pft) const
{
    const auto &states = Understory::instance().states();
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            if (states[plant.stateId()]->pft() == pft)
                return true;
        }
    }
    return false;
}

void UnderstoryCell::update(UnderstoryRU &us_ru)
{
    const auto &states = Understory::instance().states();
    mOccupied = 0;
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            mOccupied += states[plant.stateId()]->NSlots();
        }
    }
    if (mOccupied >= MaxOccupied) {
        mState = ECellState::CellFull;
        while (mOccupied > MaxOccupied) {
            // mortality due to competition. Remove pfts until space constraint is satisfied.
            UnderstoryPlant *sml = nullptr;
            int sml_slots = MaxOccupied;
            for (auto& plant : mPlants) {
                if (plant.isLiving()) {
                    if (!sml) sml = &plant;
                    if (states[sml->stateId()]->NSlots() < sml_slots) {
                        sml_slots = states[sml->stateId()]->NSlots();
                        sml = &plant;
                    }
                }
            }
            if (!sml) break;
            mOccupied = std::max(0, mOccupied - sml_slots);

            us_ru.statsPlantDied(sml);
            sml->kill();

        }
    }
    if (mOccupied == 0)
        mState = ECellState::CellEmpty;
    else
        mState = ECellState::CellFree;
}

void UnderstoryCell::growth(UnderstoryCellParams &ucp, UnderstoryRU &us_ru)
{

    const auto &us = Understory::instance();

    bool states_changed = false;
    for (auto &p : mPlants) {
        if (p.isLiving()) {
            const auto * state = us.state(p.stateId());
            const auto * pft = us.pft(state->pftIndex());
            ucp.height = state->height();

            UStateId new_id = pft->stateTransition(p, ucp, us_ru);
            if (p.stateId() != new_id) {
                // a state change!
                states_changed = true;
                p.setState(new_id);
            }

        }
    }
    if (states_changed) {
        update(us_ru);
    }
}

UnderstoryPlant* UnderstoryCell::establishment(UStateId id)
{
    for (auto& plant : mPlants) {
        if (!plant.isLiving()) {
            plant.setState(id); return &plant;
        }
    }
    return nullptr; // should not happen
}


UnderstoryStatsCell UnderstoryCell::stats() const
{
    const auto &states = Understory::instance().states();
    UnderstoryStatsCell stats;
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            const auto state = states[ plant.stateId() ];
            stats.LAI += state->LAI();
            stats.biomass += state->biomass();
            stats.height = std::max(stats.height, (float)state->height());
            stats.slotsOccupied += state->NSlots();
            stats.cellsOccupied ++;
        }
    }
    return stats;
}

UnderstoryStatsCell UnderstoryCell::stats(const UnderstoryPFT *pft) const
{
    if (!pft)
        return stats();

    const auto &states = Understory::instance().states();
    UnderstoryStatsCell stats;
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            const auto state = states[ plant.stateId() ];
            if (state->pft() == pft) {
                stats.LAI = state->LAI();
                stats.biomass = state->biomass();
                stats.height = state->height();
                stats.slotsOccupied = state->NSlots();
                stats.cellsOccupied = 1;
                return stats;
            }

        }
    }
    return stats;
}

void UnderstoryCell::addStats(QVector<UnderstoryStats> &pfts)
{
    const auto &states = Understory::instance().states();
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            const auto state = states[ plant.stateId() ];
            auto &stats = pfts[state->pftIndex()];

            stats.stats.LAI += state->LAI();
            stats.stats.biomass += state->biomass();
            stats.stats.height += state->height();
            stats.stats.cover += state->cover();
            stats.stats.slotsOccupied += state->NSlots();
            stats.stats.cellsOccupied++;

        }
    }


}



// ****************** UnderstoryRU **************************

void UnderstoryRU::setup()
{
    Q_ASSERT(mRU != nullptr);
    // create space for statistics
    mPFTStats.resize(Understory::instance().PFTs().size());

    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (auto& cell : mCells) {
        QPointF p = cellCoord(cell);
        // set state of cell to Empty for all valid 10m cells
        if (hg->constValueAt(p).isValid())
            cell.update(*this);
    }
}

void UnderstoryRU::establishment(const LightProfile &profile)
{
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();

    const double p_cell = 0.2;
    const double n_cells_represented = 1. / p_cell;

    UnderstoryCellParams ucp;
    ucp.RU = mRU;
    ucp.psiGrowingSeason = mRU->waterCycle()->meanPsiGrowingSeason();
    //ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;
    // TODO: switch to microclimate?
    ucp.meanTemperature = mRU->climate()->meanAnnualTemperature();
    ucp.lightProfile = &profile;

    for (const auto &pft : Understory::instance().PFTs()) {

        if (drandom() < pft->baseEstablishmentProbability()) {
            // analyze the pft
            ucp.PFTcalc = false;
            int isc = 0; // index on 2m cell on LIF grid
            int cell_index = 0; // index in the cell array
            for (int iy=0; iy<cPxPerRU; ++iy) {
                ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

                auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understory cells
                isc = lif_grid->index(imap.x(), imap.y()+iy);

                for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell, ++cell_index) {
                    if (ucell->isValid() &&
                        !ucell->isFull() &&
                        !ucell->hasPft(pft) &&
                        drandom() < p_cell) {

                        ucp.cell_index = cell_index;
                        ucp.ground_light = profile.ground_light[cell_index];

                        if (pft->establishment(ucp, n_cells_represented)) {
                            // the PFT establishes on the cell
                            auto p = ucell->establishment(pft->firstState());
                            if (p)
                                statsPlantEstablished(p);
                            ucell->update(*this);
                        }

                    }
                }
            }

        }
    }


}

void UnderstoryRU::growth(const LightProfile &profile)
{
    // clear stats (will be filled during growth / establishment)
    mStats.clear();
    for (auto &stat : mPFTStats)
        stat.clear();

    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();

    UnderstoryCellParams ucp;
    ucp.RU = mRU;

    //ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.psiGrowingSeason = mRU->waterCycle()->meanPsiGrowingSeason();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;
    ucp.meanTemperature = mRU->climate()->meanAnnualTemperature();
    ucp.lightProfile = &profile;

    int isc = 0; // index on 2m cell on LIF grid
    int cell_index = 0; // running index 0..2500
    for (int iy=0; iy<cPxPerRU; ++iy) {
        ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

        auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understory cells
        isc = lif_grid->index(imap.x(), imap.y()+iy);

        for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell, ++cell_index) {

            if (!ucell->isValid() || ucell->isEmpty())
                continue;

            ucp.cell_index = cell_index;
            // run growth for each plant on the cell
            ucell->growth(ucp, *this);

        }
    }


}

void UnderstoryRU::yearEnd()
{
    // collect statistics
    updateStats();

    // carbon fluxes
    if (mRU->soil()) {
        // a) turnover
        int i=0;
        double turnover_ha = 0.;
        double mortality_ha = 0.;
        for (auto &pft_stat: mPFTStats) {
            double rate = Understory::instance().state(Understory::instance().PFTs()[i]->firstState())->turnoverRate();
            turnover_ha += pft_stat.stats.biomass * rate;
            mortality_ha += pft_stat.biomass_died;
            ++i;
        }
        //
        const double understory_r_decomp = 0.14; // value of moss for permafrost
        const double understory_CNratio = 30; // same value as for moss

        double total_c_flux = turnover_ha + mortality_ha;

        CNPool litter_input( total_c_flux * biomassCFraction,
                             total_c_flux * biomassCFraction / understory_CNratio,
                            understory_r_decomp);
        mRU->snag()->addBiomassToSoil(CNPool(), litter_input);

    }


}


void UnderstoryRU::statsPlantDied(const UnderstoryPlant *p)
{
    auto *state= Understory::instance().states()[p->stateId()];
    auto &pft_stat = mPFTStats[state->pftIndex()];
    double biomass_died = state->biomass();

    pft_stat.biomass_died += biomass_died;
    ++pft_stat.died;
}

void UnderstoryRU::statsPlantEstablished(const UnderstoryPlant *p)
{
    auto *state= Understory::instance().states()[p->stateId()];
    auto &pft_stat = mPFTStats[state->pftIndex()];
    ++pft_stat.established;

}

void UnderstoryRU::statsPlantTransition(const UnderstoryPlant *p, bool growth)
{
    auto *state= Understory::instance().states()[p->stateId()];
    auto &pft_stat = mPFTStats[state->pftIndex()];

    if (growth) {
        ++pft_stat.transitionUp;
    } else {
        ++pft_stat.transitionDown;
    }

}

QPointF UnderstoryRU::cellCoord(int index)
{
    QPointF local( ( (index % cPxPerRU) + 0.5) * cPxSize, ((index/cPxPerRU) + 0.5) * cPxSize );
    return local + mRU->boundingBox().topLeft();
}

const UnderstoryCell *UnderstoryRU::cell(QPointF metric_coord) const
{
    // get index_x and index_y relative to the RU corner in 2m resolution
    QPoint p= QPoint(
        (( (int) metric_coord.x()) % cRUSize) / cPxSize,
        (( (int) metric_coord.y()) % cRUSize) / cPxSize
        );
    // get the cell from the RU level container

    int index =  p.y() * cPxPerRU +  p.x() ;
    Q_ASSERT(index>=0 && index<2500);
    return &mCells[index];
}

UnderstoryStats UnderstoryRU::stats(const UnderstoryPFT *pft) const
{
    UnderstoryStats stats;
    int n_valid = 0;
    for (auto &cell : mCells) {
        if (cell.isValid()) {
            stats.stats += cell.stats(pft);
            ++n_valid;
        }
    }
    stats.stats.calcPerRU(n_valid);
    return stats;
}

void UnderstoryRU::updateStats()
{
    // update stats

    // clear
    int n_valid = 0, n_occupied = 0;
//    for (auto &stat : mPFTStats)
//        stat.clear();

    // summarize over all cells for PFTs

    for (auto &cell : mCells) {

        if (cell.isValid()) {
            if (cell.isOccupied()) {
                ++n_occupied;
                cell.addStats(mPFTStats);
            }
            ++n_valid;

        }
    }

    // factor to calculate from a pixel count the
    // percentage of stockable area.
    float area_factor = n_valid > 0 ? 1. / n_valid : 0;
    float area_factor_ha = n_valid > 0 ? cPxPerHectare / (float)n_valid : 0;

    // summarize over all PFTs
    mStats.stats.clear();
    for (auto &pft_stat : mPFTStats) {
        // calculate values per PFT and resource unit
        pft_stat.died *= area_factor * 100.;
        pft_stat.established *= area_factor * 100.;
        pft_stat.transitionDown *= area_factor * 100.;
        pft_stat.transitionUp *= area_factor * 100.;
        pft_stat.biomass_died *= area_factor * 100.;
        // the values now are summed over all cells
        pft_stat.stats.LAI *= area_factor; // m2/m2 on stockable area
        pft_stat.stats.biomass *= area_factor_ha; // kg / (stockable) ha
        pft_stat.stats.cover *= area_factor * 100; // % cover (relative to stockable area)
        pft_stat.stats.height *= area_factor; // mean height
        pft_stat.stats.cellsOccupied *= area_factor * 100; // % area occupied by PFT
        pft_stat.stats.slotsOccupied *= area_factor * 100 / UnderstoryCell::MaxOccupied; // %occupation


        // summarise over all PFTs:
        mStats.stats += pft_stat.stats;
        mStats.died += pft_stat.died;
        mStats.established += pft_stat.established;
        mStats.transitionDown += pft_stat.transitionDown;
        mStats.transitionUp += pft_stat.transitionUp;
        mStats.biomass_died += pft_stat.biomass_died;
    }
    // special case: cells occupied is the proportion of
    // non-empty cells
    if (n_valid > 0)
        mStats.stats.cellsOccupied = 100 * n_occupied / (float)n_valid;



    QString stat_string=QString("Understory: RU: %1: ++: %2, up: %3, down: %4, --: %5")
                              .arg(mRU->index())
                              .arg(mStats.established).arg(mStats.transitionUp)
                              .arg(mStats.transitionDown).arg(mStats.died);
    //qDebug() << stat_string;
    stat_string = QString("(area/slots): %1 %, %2 %, LAI: %3, biomass: %4, height: %5")
                      .arg(mStats.stats.cellsOccupied)
                      .arg(mStats.stats.slotsOccupied)
                      .arg(mStats.stats.LAI)
                      .arg(mStats.stats.biomass)
                      .arg(mStats.stats.height);
    //qDebug() << stat_string;

}

