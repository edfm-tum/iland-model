#include "understoreyplant.h"
#include "understorey.h"

UnderstoreyPlant::UnderstoreyPlant() {}

void UnderstoreyCell::update()
{
    const auto &states = Understorey::instance().states();
    mOccupied = 0;
    for (const auto& plant : mPlants) {
        if (plant.isValid()) {
            mOccupied += states[plant.id()]->NSlots();
        }
    }
    if (mOccupied >= MaxOccupied)
        mState = ECellState::CellFull;
    if (mOccupied == 0)
        mState = ECellState::CellEmpty;
    else
        mState = ECellState::CellFree;
}

UnderstoreyStatsCell UnderstoreyCell::stats()
{
    const auto &states = Understorey::instance().states();
    UnderstoreyStatsCell stats;
    for (const auto& plant : mPlants) {
        if (plant.isValid()) {
            stats.LAI += states[ plant.id() ]->LAI();
            stats.biomass += states[ plant.id() ]->biomass();
            stats.height = std::max(stats.height, states[plant.id()]->height());
            stats.NStates ++;
        }
    }
    return stats;
}
