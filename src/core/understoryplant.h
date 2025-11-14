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
    float lif_plant; ///< lif value corrected for taller plants on the same cell
    float lif_ground; ///< light value corrected for all understory plants
    double availableNitrogen; ///< kg/ha*yr nitrogen
    //double SWCgrowingSeason; ///< relative soil water content
    double psiGrowingSeason; /// mean psi over the growing season
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


struct UnderstoryRUStats {
    void clear() { established = died = transitionDown = transitionUp = 0; ru_stats.clear(); }
    // changes
    float established { 0 }; // # of plants / cells established
    float died {0 }; // # of plants that died
    float transitionUp {0}; // # of plants with state transition to next / taller state
    float transitionDown {0}; // # of plants with state transition to previous / smaller state
    // state
    UnderstoryStatsCell ru_stats;
};

class UnderstoryRU; // forward
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

    float groundLightEffect() const { return mGroundLightEffect; }
    void resetGroundLight() { mGroundLightEffect = 1.; }

    /// summary stats for the cell
    UnderstoryStatsCell stats() const;
    /// get stats only for a given PFT
    UnderstoryStatsCell stats(const UnderstoryPFT *pft) const;

    void addStats(QVector<UnderstoryRUStats> &pfts);
private:
    /// return an array with the proportion of light
    /// reaching the plant (within understory). E.g., a value
    /// of 0.9 means that 10% of the light reaching understory is
    /// intercepted by competing vegetation
    /// same sequence as mPlants array (i.e., mPlants[i] <-> lightProfile()[i] )
    std::array<double, UnderstoryCell::NSlots> lightProfile();

    /// sum of occupation points on cell
    uint8_t mOccupied {0};
    /// current state of the cell
    ECellState mState { ECellState::CellInvalid };
    std::array<UnderstoryPlant, NSlots> mPlants;
    float mGroundLightEffect; ///< effect of cumulative LAI of plants on light at the ground
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

    // functions for statistics
    /// get pointers to the stats object for the PFT (on RU) and RU for a given plant-cell
    void statsRef(const UnderstoryPlant *p, UnderstoryRUStats **rPFTStat, UnderstoryRUStats **rRUStat);
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
    /// RU totals across all PFTs
    const UnderstoryRUStats &stats() const { return mStats; }
    /// stats for a single PFT
    UnderstoryRUStats stats(const UnderstoryPFT *pft) const;
    /// vector of states for all PFTs
    const QVector<UnderstoryRUStats> pftStats() const { return mPFTStats; }
private:
    void updateStats();
    UnderstoryRUStats mStats;
    QVector<UnderstoryRUStats> mPFTStats; ///< stats per PFT
    ResourceUnit *mRU {0};
    std::array<UnderstoryCell, cPxPerHectare> mCells;
};



#endif // UNDERSTORYPLANT_H
