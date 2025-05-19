#include "understoryplant.h"
#include "understory.h"
#include "saplings.h"
#include "speciesset.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "microclimate.h"
#include "climate.h"

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

void UnderstoryCell::addStats(QVector<UnderstoryRUStats> &pfts)
{
    const auto &states = Understory::instance().states();
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            const auto state = states[ plant.stateId() ];
            auto &stats = pfts[state->pftIndex()];

            stats.ru_stats.LAI += state->LAI();
            stats.ru_stats.biomass += state->biomass();
            stats.ru_stats.height += state->height();
            stats.ru_stats.cover += state->cover();
            stats.ru_stats.slotsOccupied += state->NSlots();
            stats.ru_stats.cellsOccupied++;

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

void UnderstoryRU::establishment()
{
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();
    const auto &speciesSet = Globals->model()->speciesSet();

    const double p_cell = 0.2;
    const double n_cells_represented = 1. / p_cell;

    UnderstoryCellParams ucp;
    ucp.RU = mRU;
    ucp.psiGrowingSeason = mRU->waterCycle()->meanPsiGrowingSeason();
    //ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;
    // TODO: switch to microclimate?
    ucp.meanTemperature = mRU->climate()->meanAnnualTemperature();

    for (const auto &pft : Understory::instance().PFTs()) {

        if (drandom() < pft->baseEstablishmentProbability()) {
            // analyze the pft
            ucp.PFTcalc = false;
            int isc = 0; // index on 2m cell on LIF grid
            for (int iy=0; iy<cPxPerRU; ++iy) {
                ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

                auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understory cells
                isc = lif_grid->index(imap.x(), imap.y()+iy);

                for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell) {
                    if (ucell->isValid() &&
                        !ucell->isFull() &&
                        !ucell->hasPft(pft) &&
                        drandom() < p_cell) {
                        float lif_value = (*lif_grid)[isc];
                        // corrected LIF value for a height of 0 (=forest floor)
                        ucp.lif_corr = speciesSet->LRIcorrection(lif_value, 0.);

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

    // as final step of a year: collect statistics
    updateStats();

}

void UnderstoryRU::growth()
{
    // clear stats (will be filled during growth / establishment)
    mStats.clear();
    for (auto &stat : mPFTStats)
        stat.clear();

    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();
    const auto &speciesSet = Globals->model()->speciesSet();


    UnderstoryCellParams ucp;
    ucp.RU = mRU;

    //ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.psiGrowingSeason = mRU->waterCycle()->meanPsiGrowingSeason();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;
    ucp.meanTemperature = mRU->climate()->meanAnnualTemperature();

    int isc = 0; // index on 2m cell on LIF grid
    for (int iy=0; iy<cPxPerRU; ++iy) {
        ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

        auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understory cells
        isc = lif_grid->index(imap.x(), imap.y()+iy);

        for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell) {

            if (!ucell->isValid())
                continue;

            float lif_value = (*lif_grid)[isc];
            // corrected LIF value for a height of 0 (=forest floor)
            ucp.lif_corr = speciesSet->LRIcorrection(lif_value, 0.);

            // run growth for each plant on the cell
            ucell->growth(ucp, *this);

        }
    }


}

void UnderstoryRU::statsRef(const UnderstoryPlant *p,
                            UnderstoryRUStats **rPFTStat,
                            UnderstoryRUStats **rRUStat)
{
    int pft_index = Understory::instance().states()[p->stateId()]->pftIndex();
    *rPFTStat = &mPFTStats[pft_index];
    *rRUStat = &mStats;
}

void UnderstoryRU::statsPlantDied(const UnderstoryPlant *p)
{
    UnderstoryRUStats *pftstat=nullptr, *rustat=nullptr;
    statsRef(p, &pftstat, &rustat);
    Q_ASSERT(pftstat != nullptr && rustat != nullptr);
    ++pftstat->died;
    ++rustat->died;
}

void UnderstoryRU::statsPlantEstablished(const UnderstoryPlant *p)
{
    UnderstoryRUStats *pftstat=nullptr, *rustat=nullptr;
    statsRef(p, &pftstat, &rustat);
    Q_ASSERT(pftstat != nullptr && rustat != nullptr);
    ++pftstat->established;
    ++rustat->established;

}

void UnderstoryRU::statsPlantTransition(const UnderstoryPlant *p, bool growth)
{
    UnderstoryRUStats *pftstat=nullptr, *rustat=nullptr;
    statsRef(p, &pftstat, &rustat);
    Q_ASSERT(pftstat != nullptr && rustat != nullptr);

    if (growth) {
        ++pftstat->transitionUp;
        ++rustat->transitionUp;
    } else {
        ++pftstat->transitionDown;
        ++rustat->transitionDown;
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

UnderstoryRUStats UnderstoryRU::stats(const UnderstoryPFT *pft) const
{
    UnderstoryRUStats stats;
    int n_valid = 0;
    for (auto &cell : mCells) {
        if (cell.isValid()) {
            stats.ru_stats += cell.stats(pft);
            ++n_valid;
        }
    }
    stats.ru_stats.calcPerRU(n_valid);
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
            if (cell.isOccupied())
                ++n_occupied;
            ++n_valid;
            cell.addStats(mPFTStats);
        }
    }

    // factor to calculate from a pixel count the
    // percentage of stockable area.
    float area_factor = n_valid > 0 ? 1. / n_valid : 0;
    float area_factor_ha = n_valid > 0 ? cPxPerHectare / (float)n_valid : 0;

    // summarize over all PFTs
    mStats.ru_stats.clear();
    for (auto &stat : mPFTStats) {
        // calculate values per PFT and resource unit
        stat.died *= area_factor * 100.;
        stat.established *= area_factor * 100.;
        stat.transitionDown *= area_factor * 100.;
        stat.transitionUp *= area_factor * 100.;
        // the values now are summed over all cells
        stat.ru_stats.LAI *= area_factor; // m2/m2 on stockable area
        stat.ru_stats.biomass *= area_factor_ha; // kg / (stockable) ha
        stat.ru_stats.cover *= area_factor * 100; // % cover (relative to stockable area)
        stat.ru_stats.height *= area_factor; // mean height
        stat.ru_stats.cellsOccupied *= area_factor * 100; // % area occupied by PFT
        stat.ru_stats.slotsOccupied *= area_factor * 100 / UnderstoryCell::MaxOccupied; // %occupation

        // summarise over all PFTs:
        mStats.ru_stats += stat.ru_stats;
        mStats.died += stat.died;
        mStats.established += stat.established;
        mStats.transitionDown += stat.transitionDown;
        mStats.transitionUp += stat.transitionUp;
    }
    // special case: cells occupied is the proportion of
    // non-empty cells
    if (n_valid > 0)
        mStats.ru_stats.cellsOccupied = 100 * n_occupied / (float)n_valid;



    QString stat_string=QString("Understory: RU: %1: ++: %2, up: %3, down: %4, --: %5")
                              .arg(mRU->index())
                              .arg(mStats.established).arg(mStats.transitionUp)
                              .arg(mStats.transitionDown).arg(mStats.died);
    //qDebug() << stat_string;
    stat_string = QString("(area/slots): %1 %, %2 %, LAI: %3, biomass: %4, height: %5")
                      .arg(mStats.ru_stats.cellsOccupied)
                      .arg(mStats.ru_stats.slotsOccupied)
                      .arg(mStats.ru_stats.LAI)
                      .arg(mStats.ru_stats.biomass)
                      .arg(mStats.ru_stats.height);
    //qDebug() << stat_string;

}

