#include "understoreyplant.h"
#include "understorey.h"
#include "saplings.h"
#include "speciesset.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "microclimate.h"

UnderstoreyPlant::UnderstoreyPlant() {}



void UnderstoreyCell::update()
{
    const auto &states = Understorey::instance().states();
    mOccupied = 0;
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            mOccupied += states[plant.stateId()]->NSlots();
        }
    }
    if (mOccupied >= MaxOccupied)
        mState = ECellState::CellFull;
    if (mOccupied == 0)
        mState = ECellState::CellEmpty;
    else
        mState = ECellState::CellFree;
}

void UnderstoreyCell::growth(UnderstoreyCellParams &ucp, UnderstoreyRUStats &stats)
{

    const auto &us = Understorey::instance();
    bool states_changed = false;
    for (auto &p : mPlants) {
        if (p.isLiving()) {
            const auto * state = us.state(p.stateId());
            const auto * pft = us.pft(state->pftIndex());


            UStateId new_id = pft->stateTransition(p, ucp, stats);
            if (p.stateId() != new_id) {
                // a state change!
                states_changed = true;
                p.setState(new_id);
            }

        }
    }
    if (states_changed) {
        update();
    }
}

void UnderstoreyCell::establishment(UStateId id)
{
    for (auto& plant : mPlants) {
        if (!plant.isLiving()) {
            plant.setState(id); break;
        }
    }
    update();
}


UnderstoreyStatsCell UnderstoreyCell::stats() const
{
    const auto &states = Understorey::instance().states();
    UnderstoreyStatsCell stats;
    for (const auto& plant : mPlants) {
        if (plant.isLiving()) {
            stats.LAI += states[ plant.stateId() ]->LAI();
            stats.biomass += states[ plant.stateId()]->biomass();
            stats.height = std::max(stats.height, states[plant.stateId()]->height());
            stats.slotsOccupied += states[plant.stateId()]->NSlots();
            stats.NStates ++;
        }
    }
    return stats;
}


// ****************** UnderstoreyRU **************************

void UnderstoreyRU::setup()
{
    Q_ASSERT(mRU != nullptr);

    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (auto& cell : mCells) {
        QPointF p = cellCoord(cell);
        // set state of cell to Empty for all valid 10m cells
        if (hg->constValueAt(p).isValid())
            cell.update();
    }
}

void UnderstoreyRU::establishment()
{
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();
    const auto &speciesSet = Globals->model()->speciesSet();

    const double p_include_pft = 0.5;
    const double p_cell = 0.1;

    UnderstoreyCellParams ucp;
    ucp.RU = mRU;
    ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;


    for (const auto &pft : Understorey::instance().PFTs()) {
        if (drandom() < p_include_pft) {
            // analyze the pft
            ucp.PFTcalc = false;
            int isc = 0; // index on 2m cell on LIF grid
            for (int iy=0; iy<cPxPerRU; ++iy) {
                ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

                auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understorey cells
                isc = lif_grid->index(imap.x(), imap.y()+iy);

                for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell) {
                    if (ucell->isValid() && !ucell->isFull() && drandom() < p_cell) {
                        float lif_value = (*lif_grid)[isc];
                        // corrected LIF value for a height of 0 (=forest floor)
                        ucp.lif_corr = speciesSet->LRIcorrection(lif_value, 0.);

                        if (pft->establishment(ucp, mStats)) {
                            // the PFT establishes on the cell
                            ucell->establishment(pft->firstState());
                        }

                    }
                }
            }

        }
    }

    // update stats
    int n_valid = 0;
    for (auto &cell : mCells) {
        if (cell.isValid()) {
            mStats.ru_stats += cell.stats();
            ++n_valid;
        }
    }
    mStats.ru_stats.calcPerRU(n_valid);

    QString stat_string=QString("Understorey: RU: %1: ++: %2, up: %3, down: %4, --: %5")
                              .arg(mRU->index())
                          .arg(mStats.established).arg(mStats.transitionUp)
                          .arg(mStats.transitionDown).arg(mStats.died);
    //qDebug() << stat_string;
    stat_string = QString("(area/slots): %1 %, %2 %, LAI: %3, biomass: %4, height: %5")
                      .arg(mStats.ru_stats.NStates)
                      .arg(mStats.ru_stats.slotsOccupied)
                      .arg(mStats.ru_stats.LAI)
                      .arg(mStats.ru_stats.biomass)
                      .arg(mStats.ru_stats.height);
    //qDebug() << stat_string;


}

void UnderstoreyRU::growth()
{
    mStats.clear();

    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    QPoint imap = mRU->cornerPointOffset(); // offset on LIF/saplings grid

    SaplingCell *sap_cells = mRU->saplingCellArray();
    const auto &speciesSet = Globals->model()->speciesSet();


    UnderstoreyCellParams ucp;
    ucp.RU = mRU;

    ucp.SWCgrowingSeason = mRU->waterCycle()->meanGrowingSeasonSWC();
    ucp.availableNitrogen = mRU->resouceUnitVariables().nitrogenAvailable;

    int isc = 0; // index on 2m cell on LIF grid
    for (int iy=0; iy<cPxPerRU; ++iy) {
        ucp.saplingCell = &sap_cells[iy*cPxPerRU]; // pointer to a row of saplings

        auto *ucell =&mCells[iy*cPxPerRU]; // pointer to a row of understorey cells
        isc = lif_grid->index(imap.x(), imap.y()+iy);

        for (int ix=0;ix<cPxPerRU; ++ix, ++ucp.saplingCell, ++isc, ++ucell) {

            if (!ucell->isValid())
                continue;

            float lif_value = (*lif_grid)[isc];
            // corrected LIF value for a height of 0 (=forest floor)
            ucp.lif_corr = speciesSet->LRIcorrection(lif_value, 0.);
            ucell->growth(ucp, mStats);
            //if (s->n_occupied() > 0)
            //    qDebug() << ucell->plants()[0].state() << lif_value << s->n_occupied() << cellCoord(*ucell);
        }
    }


}

QPointF UnderstoreyRU::cellCoord(int index)
{
    QPointF local( ( (index % cPxPerRU) + 0.5) * cPxSize, ((index/cPxPerRU) + 0.5) * cPxSize );
    return local + mRU->boundingBox().topLeft();
}

const UnderstoreyCell *UnderstoreyRU::cell(QPointF metric_coord) const
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

