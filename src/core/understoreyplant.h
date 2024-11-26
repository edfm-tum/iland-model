#ifndef UNDERSTOREYPLANT_H
#define UNDERSTOREYPLANT_H

#include <array>

using UStateId = short int;


/**
 * @brief The UnderstoreyPlant class represents
 * a single UnderstoryState on a cell.
 * The class is optimized for memory consumption.
 */
class UnderstoreyPlant
{
public:
    UnderstoreyPlant();
    UStateId id() const { return mId; }

    bool isValid() const { return mId > 0; }

    UStateId state() const { return mId; }
    void setState(UStateId new_state) { mId = new_state; }
private:
    UStateId mId {0};
};


struct UnderstoreyStatsCell {
    double LAI {0.};
    double biomass  {0.};
    double height {0.};
    int NStates {0};
    int NOccupied {0};
};

/**
 * @brief The UnderstoreyCell class
 * is container for all UnderstoreyPlant on a cell.
 * It manages occupation and stores the individual UnderstoreyPlant objects
 */
class UnderstoreyCell
{
public:
    /// number of slots per cell
    static constexpr int NSlots = 5;
    /// maximum number of occupation points per cell
    static constexpr int MaxOccupied = 10;
    enum class ECellState : uint8_t { CellInvalid=0, ///< not stockable (outside project area)
                      CellEmpty=1,   ///< the cell has no slots occupied (no plants on the cell)
                      CellFree=3,    ///< plants may establish on the cell (at least one slot occupied)
                      CellFull=4};   ///< cell is full )

    UnderstoreyCell() {};
    // acess properties
    bool isValid() const { return mState != ECellState::CellInvalid; }
    bool isFull() const { return mState == ECellState::CellFull; }
    /// updates internal data, call after content of cell changed
    void update();

    /// the plants container
    const std::array<UnderstoreyPlant, NSlots> &plants() const { return mPlants; }
    std::array<UnderstoreyPlant, NSlots> &mod_plants() { return mPlants; }


    UnderstoreyStatsCell stats();
private:
    /// sum of occupation points on cell
    uint8_t mOccupied {0};
    /// current state of the cell
    ECellState mState { ECellState::CellInvalid };
    std::array<UnderstoreyPlant, NSlots> mPlants;
};

#endif // UNDERSTOREYPLANT_H
