#ifndef FLOWMODEL_H
#define FLOWMODEL_H

#include <mutex>
#include "grid.h"

/**
 * @brief Parameters for the FlowPy gravitational mass flow simulation.
 */
struct FlowParameters {
    // --- Core Model Parameters ---
    
    /// Baseline friction angle (degrees). Serves as the macroscopic Coulomb friction coefficient (mu = tan(alpha)).
    /// Standard values range from 20 to 40 degrees depending on the natural hazard type.
    float alpha = 30.0f;

    /// Divergence exponent controlling lateral routing spread.
    /// Higher values (e.g. 100) force flow to follow the steepest descent (channelized).
    /// Lower values (e.g. 8) allow flow to spread laterally over hillsides.
    float exp = 8.0f;

    /// Minimum routing flux threshold. If a cell's routing flux drops below this,
    /// propagation stops on this path to prevent infinite spreading. Default is 0.0003.
    float flux_threshold = 0.0003f;

    /// Maximum kinetic energy height Z_delta (meters).
    /// Prevents non-physical velocity build-up in extremely long/steep channels.
    float max_z_delta = 100.0f;

    /// Toggle to skip starting/release cells that have already been traversed by a previous downhill path.
    /// Default is true (reproducing FlowPy reference behavior).
    bool skip_traversed_cells = true;

    // --- Forest Friction Parameters ---

    /// Enable or disable the effect of forest structure (FSI) on friction dissipation.
    bool forest_friction_enabled = true;

    /// Maximum additional friction angle (degrees) added to the baseline alpha in dense forest.
    float max_added_friction_forest = 10.0f;

    /// Minimum additional friction angle (degrees) added to the baseline alpha in sparse forest.
    float min_added_friction_forest = 2.0f;

    /// Velocity threshold (m/s) above which the forest friction effect reaches its minimum value.
    /// Derived to kinetic energy height threshold as z_delta = v^2 / (2 * g).
    float no_friction_effect_v = 30.0f;

    // --- Forest Detrainment Parameters ---

    /// Enable or disable the detrainment of routing flux caused by forest structure.
    bool forest_detrainment_enabled = false;

    /// Maximum detrainment rate (fraction of flux lost per step) in dense forest at rest (v=0).
    float max_added_detrainment_forest = 0.0f;

    /// Minimum detrainment rate (fraction of flux lost per step) in sparse forest.
    float min_added_detrainment_forest = 0.0f;

    /// Velocity threshold (m/s) above which the forest detrainment effect ceases.
    float no_detrainment_effect_v = 30.0f;
};

/**
 * @brief The FlowModel class is a C++ implementation of FlowPy.
 *
 * The class has minimal dependencies to iLand. It is
 * easy to use as the core implementation of the algorithm for a R or python library.
 */
class FlowModel
{
public:
    FlowModel();
    
    /// Set the digital elevation model, and the grid holding the ForestStructureIndex
    bool setup(const Grid<float> *DEM, Grid<float> &FSI);

    /// Set starting area (point, rect, or polygon)
    void setStartArea(QPointF point);
    void setStartArea(QRectF rectangle);
    void setStartArea(int standId, Grid<double> *src_grid=nullptr);

    /// Set critical infrastructure grid for vulnerability/back-calculation analysis
    void setInfrastructure(const Grid<int> &infra);

    /// Run the FlowPy simulation using the current parameters
    bool run(const QString &type, int experimentId=0);
    const QString &lastFlowType() const { return mLastType; }
    int lastExperimentId() const { return mLastExperiment; }

    // Access to parameters
    FlowParameters &parameters() { return mParams; }
    const FlowParameters &parameters() const { return mParams; }

    // Access to data grids
    const Grid<float> &dem() const { return mDEM; }
    const Grid<int> &stockable() const { return mStockable; }
    const Grid<float> &flux() const { return mFlux; }
    const Grid<float> &energy() const { return mEnergy; }
    const Grid<float> &energySum() const { return mEnergySum; }
    const Grid<float> &fpTravelAngle() const { return mFpTravelAngle; }
    const Grid<float> &slTravelAngle() const { return mSlTravelAngle; }
    const Grid<float> &fsi() const { return *mFSI; }
    const Grid<int> &infra() const { return mInfra; }
    const Grid<int> &backCalc() const { return mBackCalc; }
    const Grid<int> &count() const { return mCount; }

private:
    /// Run simulation path for a single starting cell (thread-safe execution)
    void runSingleStartCell(int startIdx);

    /// Parameters of the simulation
    FlowParameters mParams;
    QString mLastType; ///< type of flow last executed
    int mLastExperiment; ///< ID of last experiment = combination of starts & targets

    /// Grid (external) with forest structure information
    Grid<float> *mFSI { nullptr };

    /// Grids storing elevation, inputs, and outputs
    Grid<float> mDEM;
    Grid<int> mStockable; ///< 1 for stockable cells
    Grid<float> mEnergy;
    Grid<float> mFlux;
    Grid<float> mEnergySum;
    Grid<float> mFpTravelAngle;
    Grid<float> mSlTravelAngle;
    Grid<int> mInfra;
    Grid<int> mBackCalc;
    Grid<int> mCount;

    /// List of starting indices for the simulation run
    QVector<int> mStartIndices;

    /// Mutex protecting shared output grids during the reduction phase
    std::mutex mOutputMutex;
};

#endif // FLOWMODEL_H
