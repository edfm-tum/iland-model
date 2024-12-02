#ifndef UNDERSTOREYPLANT_H
#define UNDERSTOREYPLANT_H

#include <array>
#include <QPointF>

#include "globalsettings.h"

using UStateId = short int;

class ResourceUnit; // forward
struct SaplingCell; // forward
struct UnderstoreyCellParams {
    ResourceUnit *RU; ///< pointer to resource unit
    SaplingCell *saplingCell; ///< corresponding sapling cell
    float lif_corr; ///< corrected LIF value at the 2m cell
    double availableNitrogen; ///< kg/ha*yr nitrogen
    double SWCgrowingSeason; ///< relative soil water content
    bool PFTcalc { false };
    double nitrogenResponse;
    double waterResponse;
};


/**
 * @brief The UnderstoreyPlant class represents
 * a single UnderstoryState on a cell.
 * The class is optimized for memory consumption.
 */
class UnderstoreyPlant
{
public:
    UnderstoreyPlant();

    bool isLiving() const { return mId < std::numeric_limits<UStateId>::max(); }

    /// the stateId is a unique number for a state, and also the index
    /// within the list of possible states. Growing to the next state is therefore just stateId() + 1
    UStateId stateId() const { return mId; }
    void setState(UStateId new_state) { mId = new_state; }
private:
    UStateId mId { std::numeric_limits<UStateId>::max() };
};

struct UnderstoreyStatsCell {
    void operator+=(const UnderstoreyStatsCell &rSide) {
        LAI += rSide.LAI; // total LAI
        biomass += rSide.biomass; // total biomass
        height = std::max(height, rSide.height); // maximum height
        slotsOccupied += rSide.slotsOccupied; // count total degree slots
        NStates += rSide.NStates > 0 ? 1 : 0; // count occupied cells
    }
    void calcPerRU(int n_valid) {
        if (n_valid>0) {
            LAI = LAI / (double)n_valid;
            biomass = biomass / (double)n_valid;
            NStates = NStates / (double)n_valid * 100; // % occupied
            slotsOccupied = slotsOccupied/ (double)n_valid * 100; // % total of slots occupied
        }
    }
    void clear() {LAI=0.; biomass=0.; height=0.; NStates=0; slotsOccupied = 0; }
    double LAI {0.};
    double biomass  {0.};
    double height {0.};
    int NStates {0};
    int slotsOccupied {0};
};


struct UnderstoreyRUStats {
    void clear() { established = died = transitionDown = transitionUp = 0; ru_stats.clear(); }
    // changes
    int established { 0 }; // # of plants / cells established
    int died {0 }; // # of plants that died
    int transitionUp {0}; // # of plants with state transition to next / taller state
    int transitionDown {0}; // # of plants with state transition to previous / smaller state
    // state
    UnderstoreyStatsCell ru_stats;
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


    void growth(UnderstoreyCellParams &ucp, UnderstoreyRUStats &stats);
    void establishment(UStateId id);

    /// the plants container
    const std::array<UnderstoreyPlant, NSlots> &plants() const { return mPlants; }
    std::array<UnderstoreyPlant, NSlots> &mod_plants() { return mPlants; }


    UnderstoreyStatsCell stats() const;
private:
    /// sum of occupation points on cell
    uint8_t mOccupied {0};
    /// current state of the cell
    ECellState mState { ECellState::CellInvalid };
    std::array<UnderstoreyPlant, NSlots> mPlants;
};

/**
 * @brief The UnderstoreyRU class
 * holds the actual understorey per resource unit (an array of UnderstoreyCell).
 *
 */
class UnderstoreyRU
{
public:
    // setup
    void setup();
    void setRU(ResourceUnit* ru) {mRU = ru; }

    // actions
    void establishment();
    void growth();

    // access
    /// get metric coordinates (landscape) of a cell with given index
    QPointF cellCoord(int index);
    /// get metric coordinates (landscape) of a cell
    QPointF cellCoord(const UnderstoreyCell& cell) { return cellCoord( &cell - mCells.begin());}
    /// get cell at given coordinates (metric)
    /// Note that selecting the right RU is
    /// done by Understorey::cell()!
    const UnderstoreyCell *cell(QPointF metric_coord) const;
private:
    UnderstoreyRUStats mStats;
    ResourceUnit *mRU {0};
    std::array<UnderstoreyCell, cPxPerHectare> mCells;
};



#endif // UNDERSTOREYPLANT_H
