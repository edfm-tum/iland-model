#ifndef UNDERSTORYPLANT_H
#define UNDERSTORYPLANT_H

#include <array>
#include <QPointF>

#include "globalsettings.h"

using UStateId = short int;

class ResourceUnit; // forward
struct SaplingCell; // forward
class UnderstoryPFT; // forward
struct UnderstoryCellParams {
    ResourceUnit *RU; ///< pointer to resource unit
    SaplingCell *saplingCell; ///< corresponding sapling cell
    float lif_corr; ///< corrected LIF value at the 2m cell
    double availableNitrogen; ///< kg/ha*yr nitrogen
    double SWCgrowingSeason; ///< relative soil water content
    double meanTemperature; ///< mean annual temp (micro or macro)
    bool PFTcalc { false };
    double nitrogenResponse;
    double waterResponse;
    double tempResponse;
};


/**
 * @brief The UnderstoryPlant class represents
 * a single UnderstoryState on a cell.
 * The class is optimized for memory consumption.
 */
class UnderstoryPlant
{
public:
    UnderstoryPlant();

    bool isLiving() const { return mId < std::numeric_limits<UStateId>::max(); }
    void kill() { mId = std::numeric_limits<UStateId>::max(); }

    /// the stateId is a unique number for a state, and also the index
    /// within the list of possible states. Growing to the next state is therefore just stateId() + 1
    UStateId stateId() const { return mId; }
    void setState(UStateId new_state) { mId = new_state; }
private:
    UStateId mId { std::numeric_limits<UStateId>::max() };
};

struct UnderstoryStatsCell {
    void operator+=(const UnderstoryStatsCell &rSide) {
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


struct UnderstoryRUStats {
    void clear() { established = died = transitionDown = transitionUp = 0; ru_stats.clear(); }
    // changes
    int established { 0 }; // # of plants / cells established
    int died {0 }; // # of plants that died
    int transitionUp {0}; // # of plants with state transition to next / taller state
    int transitionDown {0}; // # of plants with state transition to previous / smaller state
    // state
    UnderstoryStatsCell ru_stats;
};

/**
 * @brief The UnderstoryCell class
 * is container for all UnderstoryPlant on a cell.
 * It manages occupation and stores the individual UnderstoryPlant objects
 */
class UnderstoryCell
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

    UnderstoryCell() {};
    // acess properties
    bool isValid() const { return mState != ECellState::CellInvalid; }
    bool isFull() const { return mState == ECellState::CellFull; }
    /// updates internal data, call after content of cell changed
    void update(UnderstoryRUStats &stats);


    void growth(UnderstoryCellParams &ucp, UnderstoryRUStats &stats);
    void establishment(UStateId id);

    /// the plants container
    const std::array<UnderstoryPlant, NSlots> &plants() const { return mPlants; }
    std::array<UnderstoryPlant, NSlots> &mod_plants() { return mPlants; }


    /// summary stats for the cell
    UnderstoryStatsCell stats() const;
    /// get stats only for a given PFT
    UnderstoryStatsCell stats(const UnderstoryPFT *pft) const;
private:
    /// sum of occupation points on cell
    uint8_t mOccupied {0};
    /// current state of the cell
    ECellState mState { ECellState::CellInvalid };
    std::array<UnderstoryPlant, NSlots> mPlants;
};

/**
 * @brief The UnderstoryRU class
 * holds the actual understory per resource unit (an array of UnderstoryCell).
 *
 */
class UnderstoryRU
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
    QPointF cellCoord(const UnderstoryCell& cell) { return cellCoord( &cell - mCells.begin());}
    /// get cell at given coordinates (metric)
    /// Note that selecting the right RU is
    /// done by Understory::cell()!
    const UnderstoryCell *cell(QPointF metric_coord) const;
    const UnderstoryRUStats &stats() const { return mStats; }
    UnderstoryRUStats stats(const UnderstoryPFT *pft) const;
private:
    UnderstoryRUStats mStats;
    ResourceUnit *mRU {0};
    std::array<UnderstoryCell, cPxPerHectare> mCells;
};



#endif // UNDERSTORYPLANT_H
