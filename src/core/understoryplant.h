#ifndef UNDERSTORYPLANT_H
#define UNDERSTORYPLANT_H

#include <array>
#include <QPointF>

#include "globalsettings.h"
#include "saplings.h"

using UStateId = short int;

class ResourceUnit; // forward
class UnderstoryPFT; // forward
struct UnderstoryCellParams {
    ResourceUnit *RU; ///< pointer to resource unit
    SaplingCell *saplingCell; ///< corresponding sapling cell
    double availableNitrogen; ///< kg/ha*yr nitrogen
    //double SWCgrowingSeason; ///< relative soil water content
    double psiGrowingSeason; /// mean psi over the growing season
    double meanTemperature; ///< mean annual temp (micro or macro)
    bool PFTcalc { false };
    double nitrogenResponse;
    double waterResponse;
    double tempResponse;
    const LightProfile *lightProfile; ///< pre-calculated light profile for resource unit
    // cached variables
    float height; /// height of the state (m)
    int cell_index; ///< currently processed cell
    float ground_light; ///< light on the forest floor
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
    /// the lowest state (after establishmeht) is pft.firstState()
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
        cover += rSide.cover;
        slotsOccupied += rSide.slotsOccupied; // count total degree slots
        cellsOccupied += rSide.cellsOccupied; // count occupied cells
    }
    void calcPerRU(int n_valid) {
        if (n_valid>0) {
            LAI = LAI / (float)n_valid;
            biomass = biomass / (float)n_valid;
            cellsOccupied = cellsOccupied / (float)n_valid * 100; // % occupied
            slotsOccupied = slotsOccupied/ (float)n_valid * 100; // % total of slots occupied
        }
    }
    void clear() {LAI=0.; biomass=0.; height=0.; cellsOccupied=0; slotsOccupied = 0; }
    float LAI {0.}; ///< cell: LAI (m2/m2) from state, RU: LAI (stockable area)
    float biomass  {0.};
    float height {0.};
    float cover {0.}; ///< %cover (state-variable)
    float cellsOccupied {0}; ///< Cell: 1/0, RU: % cells covered
    float slotsOccupied {0}; ///< Cell: N Slots, RU: % slots covered
};

/** @class UnderstoryStats is a containter for understory statistics on different aggregation levels.
    Used for sums per PFT (on RU), sums per RU, and total value (landscape).
    Values on RU are always expressed as per ha stockable area (i.e., half-stockable RUs are scaled up) (see UnterstoryRU::updateStats())
 */
struct UnderstoryStats {
    void clear() { established = died = transitionDown = transitionUp = 0; biomass_died=0.; stats.clear(); }
    // changes
    float established { 0 }; // # of plants / cells established
    float died {0 }; // # of plants that died
    float transitionUp {0}; // # of plants with state transition to next / taller state
    float transitionDown {0}; // # of plants with state transition to previous / smaller state
    // state
    UnderstoryStatsCell stats;
    // carbon flux
    float biomass_died {0.}; // total biomass
};

class UnderstoryRU; // forward
/**
 * @brief The UnderstoryCell class
 * is container for all UnderstoryPlant on a cell.
 * It manages occupation and stores the individual UnderstoryPlant objects
 */
class alignas(64) UnderstoryCell
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
    bool isEmpty() const { return mState == ECellState::CellEmpty; }
    /// is the cell stockable?
    bool isValid() const { return mState != ECellState::CellInvalid; }
    /// are all slots used?
    bool isFull() const { return mState == ECellState::CellFull; }
    /// is at least on slot occupied?
    bool isOccupied() const {return mState == ECellState::CellFree || isFull(); }
    /// checks if the given pft is already present on the cell (returns true in that case)
    bool hasPft(const UnderstoryPFT *pft) const;
    /// updates internal data, call after content of cell changed
    void update(UnderstoryRU &us_ru);


    void growth(UnderstoryCellParams &ucp, UnderstoryRU &us_ru);
    UnderstoryPlant *establishment(UStateId id);

    /// the plants container
    const std::array<UnderstoryPlant, NSlots> &plants() const { return mPlants; }
    std::array<UnderstoryPlant, NSlots> &mod_plants() { return mPlants; }

    /// summary stats for the cell
    UnderstoryStatsCell stats() const;
    /// get stats only for a given PFT
    UnderstoryStatsCell stats(const UnderstoryPFT *pft) const;

    void addStats(QVector<UnderstoryStats> &pfts);
private:
    /// sum of occupation points on cell
    uint8_t mOccupied {0};
    /// current state of the cell
    ECellState mState { ECellState::CellInvalid };
    std::array<UnderstoryPlant, NSlots> mPlants;
};

class UnderstoryState; // forward
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

    /// establishment of understory plants on the RU
    void establishment(const LightProfile &profile);
    /// growth (and death) of understory on the RU
    void growth(const LightProfile &profile);
    /// finalize year, carbon fluxes
    void yearEnd();

    // functions for statistics
    /// get pointers to the stats object for the PFT (on RU) and RU for a given plant-cell
    void statsPlantDied(const UnderstoryPlant *p);
    void statsPlantEstablished(const UnderstoryPlant *p);
    void statsPlantTransition(const UnderstoryPlant *p, bool growth);

    // access
    const ResourceUnit *ru() const { return mRU; }
    /// get metric coordinates (landscape) of a cell with given index
    QPointF cellCoord(int index);
    /// get metric coordinates (landscape) of a cell
    QPointF cellCoord(const UnderstoryCell& cell) { return cellCoord( &cell - mCells.begin());}
    /// get cell at given coordinates (metric)
    /// Note that selecting the right RU is
    /// done by Understory::cell()!
    const UnderstoryCell *cell(QPointF metric_coord) const;
    /// cet cell by index
    const UnderstoryCell *cell(int index) const { return &mCells[index]; }

    /// RU totals across all PFTs
    const UnderstoryStats &stats() const { return mStats; }
    /// stats for a single PFT
    UnderstoryStats stats(const UnderstoryPFT *pft) const;
    /// vector of states for all PFTs
    const QVector<UnderstoryStats> pftStats() const { return mPFTStats; }
private:
    /// accumulate data for RU level, scale to stockable area
    void updateStats();
    UnderstoryStats mStats;
    QVector<UnderstoryStats> mPFTStats; ///< stats per PFT
    ResourceUnit *mRU {0};
    std::array<UnderstoryCell, cPxPerHectare> mCells;
};



#endif // UNDERSTORYPLANT_H
