#include "flowmodel.h"
#include "debugtimer.h"
#include "globalsettings.h"
#include "model.h"
#include "mapgrid.h"
#include <QtConcurrent/QtConcurrent>
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
struct SimCell {
    int index;
    float flux;
    float z_delta;
    float altitude;
    int parents[8];
    int parent_count;
    float min_distance;
    float max_gamma;
    float sl_gamma;
    int back_calc_val;
    bool is_start;
};
}

FlowModel::FlowModel() {}

bool FlowModel::setup(const Grid<float> *DEM, Grid<float> &FSI)
{
    mFSI = &FSI;

    mEnergy.setup(FSI.metricRect(), FSI.cellsize());
    mFlux.setup(FSI.metricRect(), FSI.cellsize());
    mDEM.setup(FSI.metricRect(), FSI.cellsize());
    mStockable.setup(FSI.metricRect(), FSI.cellsize());
    mEnergySum.setup(FSI.metricRect(), FSI.cellsize());
    mFpTravelAngle.setup(FSI.metricRect(), FSI.cellsize());
    mSlTravelAngle.setup(FSI.metricRect(), FSI.cellsize());
    mInfra.setup(FSI.metricRect(), FSI.cellsize());
    mBackCalc.setup(FSI.metricRect(), FSI.cellsize());
    mCount.setup(FSI.metricRect(), FSI.cellsize());

    // initialize output grids
    mEnergy.initialize(0.f);
    mFlux.initialize(0.f);
    mEnergySum.initialize(0.f);
    mFpTravelAngle.initialize(0.f);
    mSlTravelAngle.initialize(90.f);
    mInfra.initialize(0);
    mBackCalc.initialize(0);
    mCount.initialize(0);

    // copy elevation from DEM grid, but make sure the size matches exactly
    for (auto *p = mDEM.begin(); p != mDEM.end(); ++p) {
        auto pt = mDEM.cellCenterPoint(p);
        if (DEM->coordValid(pt))
            *p = DEM->constValueAt(pt);
        else
            *p = 0.;
    }
    // copy stockable area
    const auto &hg = GlobalSettings::instance()->model()->heightGrid();
    for (int i=0;i<hg->count();++i) {
        mStockable[i] =  (*hg)[i].isValid() ? 1 : 0;
    }

    qDebug() << "*** Setup of Flow module  ***";
    qDebug() << "Landscape: " << mFSI->rectangle() << ", cellsize:" << mFSI->cellsize();

    return true;
}

void FlowModel::setInfrastructure(const Grid<int> &infra)
{
    mInfra.initialize(0);
    int total_cells = 0;
    if (infra.count() == mInfra.count() && infra.metricRect() == mInfra.metricRect()) {
        mInfra.copy(infra);
        for (int i = 0; i < mInfra.count(); ++i) {
            if (mInfra[i] > 0) total_cells++;
        }
        qDebug() << "FlowModel::setInfrastructure: direct copy. Count of infrastructure cells:" << total_cells;
    } else {
        for (int i = 0; i < mInfra.count(); ++i) {
            QPointF p = mInfra.cellCenterPoint(i);
            QPoint idx = infra.indexAt(p);
            if (infra.isIndexValid(idx)) {
                int val = infra.constValueAtIndex(idx);
                mInfra[i] = val;
                if (val > 0) total_cells++;
            }
        }
        qDebug() << "FlowModel::setInfrastructure: spatial copy (size mismatch). Count of infrastructure cells:" << total_cells;
    }
}

void FlowModel::setStartArea(QPointF point)
{
    mStartIndices.clear();
    QPoint idx = mFSI->indexAt(point);
    if (mFSI->isIndexValid(idx)) {
        mStartIndices.append(mFSI->index(idx));
    }
}

void FlowModel::setStartArea(QRectF rectangle)
{
    mStartIndices.clear();
    QPoint p_min = mFSI->indexAt(rectangle.topLeft());
    QPoint p_max = mFSI->indexAt(rectangle.bottomRight());
    mFSI->validate(p_min);
    mFSI->validate(p_max);
    for (int y = qMin(p_min.y(), p_max.y()); y <= qMax(p_min.y(), p_max.y()); ++y) {
        for (int x = qMin(p_min.x(), p_max.x()); x <= qMax(p_min.x(), p_max.x()); ++x) {
            if (mFSI->isIndexValid(x, y)) {
                mStartIndices.append(mFSI->index(x, y));
            }
        }
    }
}

void FlowModel::setStartArea(int standId, Grid<double>* src_grid)
{
    mStartIndices.clear();

    // if provided, use the custom grid
    if (src_grid) {
        if (src_grid->isEmpty())
            throw IException(QString("FlowModule: setStartArea for grid failed: Grid is not valid or loaded."));
        if (src_grid->metricRect() != dem().metricRect())
            throw IException(QString("FlowModule: setStartArea for grid failed: Grid has a invalid extent."));
        for (int idx = 0;idx < src_grid->count(); ++idx) {
            if (src_grid->valueAtIndex(idx) == standId) {
                mStartIndices.append(idx);
            }
        }
        return;

    }
    // use the internal stand grid
    const MapGrid *standGrid = GlobalSettings::instance()->model()->standGrid();
    if (!standGrid || !standGrid->isValid()) {
        throw IException(QString("FlowModule: setStartArea for stand %1 failed: Stand Grid is not valid or loaded.").arg(standId));
    }
    const QList<int> indices = standGrid->gridIndices(standId);
    for (int idx : indices) {
        if (mFSI->isIndexValid(mFSI->indexOf(idx))) {
            mStartIndices.append(idx);
        }
    }
}

bool FlowModel::run(const QString &type, int experimentId)
{
    mLastType = type;
    mLastExperiment = experimentId;
    qDebug() << "Flow Model running for" << type << "with starting cells count:" << mStartIndices.size();

    DebugTimer t("flow");
    // Sort starting cells by elevation descending to match Python flowpy logic
    std::sort(mStartIndices.begin(), mStartIndices.end(), [this](int a, int b) {
        return mDEM[a] > mDEM[b];
    });

    // Reset output grids
    mEnergy.initialize(0.f);
    mFlux.initialize(0.f);
    mEnergySum.initialize(0.f);
    mFpTravelAngle.initialize(0.f);
    mSlTravelAngle.initialize(90.f);
    mBackCalc.initialize(0);
    mCount.initialize(0);

    // Run simulations concurrently using ThreadRunner (respects global settings)
    GlobalSettings::instance()->model()->threadExec().run([this](int startIdx) {
        // Skip traversed start cells (already reached by previous flow paths)
        {
            std::lock_guard<std::mutex> lock(mOutputMutex);
            if (mEnergy[startIdx] > 0.0f) {
                return;
            }
        }

        runSingleStartCell(startIdx);
    }, mStartIndices);

    double time_ms = t.elapsed();
    double px_per_s = mStartIndices.size() / (time_ms > 0 ? time_ms : 1) * 1000.;
    qDebug() << "Flow Model finished simulation, " << px_per_s << " starting cells/s.";
    return true;
}

void FlowModel::runSingleStartCell(int startIdx)
{
    int sizeX = mDEM.sizeX();
    float cellsize = mDEM.cellsize();

    std::vector<SimCell> cell_list;
    cell_list.reserve(500);

    // thread_local index map avoids re-allocating memory in concurrent loops
    thread_local static std::vector<int> cell_list_index_map;
    if (cell_list_index_map.size() != (size_t)mDEM.count()) {
        cell_list_index_map.assign(mDEM.count(), -1);
    }

    // Create and add the initial cell
    SimCell start_cell;
    start_cell.index = startIdx;
    start_cell.flux = 1.0f;
    start_cell.z_delta = 0.0f;
    start_cell.altitude = mDEM[startIdx];
    start_cell.parent_count = 0;
    start_cell.min_distance = 0.0f;
    start_cell.max_gamma = 0.0f;
    start_cell.sl_gamma = 90.0f;
    start_cell.back_calc_val = 0;
    start_cell.is_start = true;

    cell_list.push_back(start_cell);
    cell_list_index_map[startIdx] = 0;

    size_t idx = 0;
    while (idx < cell_list.size()) {
        SimCell& cell = cell_list[idx];

        QPoint pos = mDEM.indexOf(cell.index);

        // Moore Neighborhood valid check
        bool ng_valid = true;
        int ng_indices[9];
        float ng_elevations[9];
        int ng_count = 0;

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = pos.x() + dx;
                int ny = pos.y() + dy;
                if (!mDEM.isIndexValid(nx, ny)) {
                    ng_valid = false;
                    break;
                }
                int n_idx = mDEM.index(nx, ny);
                float elev = mDEM[n_idx];
                if (mDEM.isNull(elev)) {
                    ng_valid = false;
                    break;
                }
                ng_indices[ng_count] = n_idx;
                ng_elevations[ng_count] = elev;
                ng_count++;
            }
            if (!ng_valid) break;
        }

        if (!ng_valid) {
            idx++;
            continue;
        }

        // Calculate travel angles if not start cell
        if (!cell.is_start && cell.parent_count > 0) {
            float min_dist = std::numeric_limits<float>::max();
            float start_alt = cell_list[0].altitude;
            for (int p = 0; p < cell.parent_count; ++p) {
                const SimCell& parent = cell_list[cell.parents[p]];
                int dx = std::abs((parent.index % sizeX) - (cell.index % sizeX));
                int dy = std::abs((parent.index / sizeX) - (cell.index / sizeX));
                float d = std::sqrt(dx*dx + dy*dy) * cellsize + parent.min_distance;
                if (d < min_dist) {
                    min_dist = d;
                }
            }
            cell.min_distance = min_dist;
            float dh = start_alt - cell.altitude;
            cell.max_gamma = (min_dist > 0.0f) ? (std::atan(dh / min_dist) * 180.0f / M_PI) : 0.0f;

            // Straight-line travel angle
            const SimCell& start_c = cell_list[0];
            int dx_sl = std::abs((start_c.index % sizeX) - (cell.index % sizeX));
            int dy_sl = std::abs((start_c.index / sizeX) - (cell.index / sizeX));
            float ds_sl = std::sqrt(dx_sl*dx_sl + dy_sl*dy_sl) * cellsize;
            cell.sl_gamma = (ds_sl > 0.0f) ? (std::atan(dh / ds_sl) * 180.0f / M_PI) : 90.0f;
        }

        // 1. calc_z_delta()
        float z_delta_ng[9];
        float altitude = ng_elevations[4]; // focal cell
        float forest = mFSI ? (*mFSI)[cell.index] : 0.0f;
        float alpha_calc = mParams.alpha;

        if (mParams.forest_friction_enabled && forest > 0.0f) {
            float no_friction_effect_zdelta = (mParams.no_friction_effect_v * mParams.no_friction_effect_v) / (2.0f * 9.8f);
            if (cell.z_delta < no_friction_effect_zdelta) {
                float rest = mParams.max_added_friction_forest * forest;
                float slope = - (rest - mParams.min_added_friction_forest) / no_friction_effect_zdelta;
                float friction = slope * cell.z_delta + rest;
                if (friction < mParams.min_added_friction_forest) {
                    friction = mParams.min_added_friction_forest;
                }
                alpha_calc += std::max(0.0f, friction);
            } else {
                alpha_calc += mParams.min_added_friction_forest;
            }
        }
        float tan_alpha = std::tan(alpha_calc * M_PI / 180.0f);

        const float ds[9] = { 1.41421356f, 1.0f, 1.41421356f,
                              1.0f,         0.0f, 1.0f,
                              1.41421356f, 1.0f, 1.41421356f };

        for (int i = 0; i < 9; ++i) {
            float z_gamma = altitude - ng_elevations[i];
            float z_alpha = ds[i] * cellsize * tan_alpha;
            float val = cell.z_delta + z_gamma - z_alpha;
            if (val < 0.0f) val = 0.0f;
            if (val > mParams.max_z_delta) val = mParams.max_z_delta;
            z_delta_ng[i] = val;
        }

        // 2. calc_persistence()
        float persistence[9] = {0.0f};
        bool no_flow[9] = {true, true, true,
                           true, true, true,
                           true, true, true};
        no_flow[4] = false; // no flow to self

        if (cell.is_start || cell.parent_count == 0) {
            for (int i = 0; i < 9; ++i) {
                persistence[i] = 1.0f;
            }
        } else {
            bool parent_is_start = false;
            for (int p = 0; p < cell.parent_count; ++p) {
                if (cell_list[cell.parents[p]].is_start) {
                    parent_is_start = true;
                    break;
                }
            }
            if (parent_is_start) {
                for (int i = 0; i < 9; ++i) {
                    persistence[i] = 1.0f;
                }
            } else {
                for (int p = 0; p < cell.parent_count; ++p) {
                    const SimCell& parent = cell_list[cell.parents[p]];
                    int dx = (parent.index % sizeX) - (cell.index % sizeX);
                    int dy = (parent.index / sizeX) - (cell.index / sizeX);

                    int p_row = dy + 1;
                    int p_col = dx + 1;
                    if (p_row >= 0 && p_row < 3 && p_col >= 0 && p_col < 3) {
                        no_flow[p_row * 3 + p_col] = false;
                    }

                    float maxweight = parent.z_delta;

                    if (dx == -1 && dy == -1) {
                        persistence[8] += maxweight; // NE
                        persistence[7] += 0.707f * maxweight; // N
                        persistence[5] += 0.707f * maxweight; // E
                    } else if (dx == -1 && dy == 0) {
                        persistence[5] += maxweight; // E
                        persistence[8] += 0.707f * maxweight; // NE
                        persistence[2] += 0.707f * maxweight; // SE
                    } else if (dx == -1 && dy == 1) {
                        persistence[2] += maxweight; // SE
                        persistence[1] += 0.707f * maxweight; // S
                        persistence[5] += 0.707f * maxweight; // E
                    } else if (dx == 0 && dy == -1) {
                        persistence[7] += maxweight; // N
                        persistence[6] += 0.707f * maxweight; // NW
                        persistence[8] += 0.707f * maxweight; // NE
                    } else if (dx == 0 && dy == 1) {
                        persistence[1] += maxweight; // S
                        persistence[0] += 0.707f * maxweight; // SW
                        persistence[2] += 0.707f * maxweight; // SE
                    } else if (dx == 1 && dy == -1) {
                        persistence[6] += maxweight; // NW
                        persistence[3] += 0.707f * maxweight; // W
                        persistence[7] += 0.707f * maxweight; // N
                    } else if (dx == 1 && dy == 0) {
                        persistence[3] += maxweight; // W
                        persistence[0] += 0.707f * maxweight; // SW
                        persistence[6] += 0.707f * maxweight; // NW
                    } else if (dx == 1 && dy == 1) {
                        persistence[0] += maxweight; // SW
                        persistence[1] += 0.707f * maxweight; // S
                        persistence[3] += 0.707f * maxweight; // W
                    }
                }
            }
        }

        for (int i = 0; i < 9; ++i) {
            if (!no_flow[i]) {
                persistence[i] = 0.0f;
            }
        }

        // 3. calc_tanbeta()
        float tan_beta[9] = {0.0f};
        float sum_tan_beta = 0.0f;
        const float ds_beta[9] = { 1.41421356f, 1.0f, 1.41421356f,
                                   1.0f,         1.0f, 1.0f,
                                   1.41421356f, 1.0f, 1.41421356f };

        for (int i = 0; i < 9; ++i) {
            if (i == 4) continue;
            if (z_delta_ng[i] <= 0.0f) continue;
            if (persistence[i] <= 0.0f) continue;

            float slope_angle = std::atan((altitude - ng_elevations[i]) / (ds_beta[i] * cellsize));
            float beta = slope_angle + M_PI / 2.0f;
            tan_beta[i] = std::tan(beta / 2.0f);
            sum_tan_beta += tan_beta[i];
        }

        // 4. calc_distribution()
        float r_t[9] = {0.0f};
        if (sum_tan_beta > 0.0f) {
            float sum_rt_exp = 0.0f;
            float rt_exp[9] = {0.0f};
            for (int i = 0; i < 9; ++i) {
                if (tan_beta[i] > 0.0f) {
                    rt_exp[i] = std::pow(tan_beta[i], mParams.exp);
                    sum_rt_exp += rt_exp[i];
                }
            }
            if (sum_rt_exp > 0.0f) {
                for (int i = 0; i < 9; ++i) {
                    r_t[i] = rt_exp[i] / sum_rt_exp;
                }
            }
        }

        // Detrainment
        float detrainment = 0.0f;
        if (mParams.forest_detrainment_enabled && forest > 0.0f) {
            float no_detrainment_effect_zdelta = (mParams.no_detrainment_effect_v * mParams.no_detrainment_effect_v) / (2.0f * 9.8f);
            float rest = mParams.max_added_detrainment_forest * forest;
            float slope = - (rest - mParams.min_added_detrainment_forest) / no_detrainment_effect_zdelta;
            detrainment = slope * cell.z_delta + rest;
            if (detrainment < mParams.min_added_detrainment_forest) {
                detrainment = mParams.min_added_detrainment_forest;
            }
            detrainment = std::max(0.0f, detrainment);
        }

        float active_flux = std::max(0.0003f, cell.flux - detrainment);

        float dist[9] = {0.0f};
        float sum_pers_rt = 0.0f;
        for (int i = 0; i < 9; ++i) {
            sum_pers_rt += persistence[i] * r_t[i];
        }

        if (sum_pers_rt > 0.0f) {
            for (int i = 0; i < 9; ++i) {
                dist[i] = (persistence[i] * r_t[i]) / sum_pers_rt * active_flux;
            }
        }

        // Apply threshold and corrected flux conservation (using count_alive)
        float threshold = mParams.flux_threshold;
        int count_alive = 0;
        float mass_to_distribute = 0.0f;
        for (int i = 0; i < 9; ++i) {
            if (dist[i] > threshold) {
                count_alive++;
            } else if (dist[i] > 0.0f) {
                mass_to_distribute += dist[i];
            }
        }

        if (mass_to_distribute > 0.0f && count_alive > 0) {
            float extra = mass_to_distribute / count_alive;
            for (int i = 0; i < 9; ++i) {
                if (dist[i] > threshold) {
                    dist[i] += extra;
                } else {
                    dist[i] = 0.0f;
                }
            }
        }

        float sum_dist = 0.0f;
        for (int i = 0; i < 9; ++i) sum_dist += dist[i];
        if (sum_dist < active_flux && count_alive > 0) {
            float extra = (active_flux - sum_dist) / count_alive;
            for (int i = 0; i < 9; ++i) {
                if (dist[i] > threshold) {
                    dist[i] += extra;
                }
            }
        }



        // Generate target cells
        struct TargetNeighbor {
            int index;
            float dist;
            float z_delta;
            float elevation;
        };

        std::vector<TargetNeighbor> targets;
        for (int i = 0; i < 9; ++i) {
            if (i == 4) continue;
            if (dist[i] > threshold) {
                targets.push_back({ng_indices[i], dist[i], z_delta_ng[i], ng_elevations[i]});
            }
        }

        // Sort targets by z_delta ascending to match Python zip sorted
        std::sort(targets.begin(), targets.end(), [](const TargetNeighbor& a, const TargetNeighbor& b) {
            return a.z_delta < b.z_delta;
        });

        for (const auto& target : targets) {
            int pos = cell_list_index_map[target.index];
            if (pos >= (int)idx) {
                SimCell& target_cell = cell_list[pos];
                target_cell.flux += target.dist;
                bool parent_exists = false;
                for (int p = 0; p < target_cell.parent_count; ++p) {
                    if (target_cell.parents[p] == (int)idx) {
                        parent_exists = true;
                        break;
                    }
                }
                if (!parent_exists && target_cell.parent_count < 8) {
                    target_cell.parents[target_cell.parent_count++] = (int)idx;
                }
                if (target.z_delta > target_cell.z_delta) {
                    target_cell.z_delta = target.z_delta;
                }
            } else {
                SimCell new_cell;
                new_cell.index = target.index;
                new_cell.flux = target.dist;
                new_cell.z_delta = target.z_delta;
                new_cell.altitude = target.elevation;
                new_cell.parents[0] = (int)idx;
                new_cell.parent_count = 1;
                new_cell.min_distance = 0.0f;
                new_cell.max_gamma = 0.0f;
                new_cell.sl_gamma = 90.0f;
                new_cell.back_calc_val = 0;
                new_cell.is_start = false;

                cell_list.push_back(new_cell);
                cell_list_index_map[target.index] = cell_list.size() - 1;
            }
        }

        idx++;
    }

    // Back-calculation single-pass backward propagation
    for (int i = (int)cell_list.size() - 1; i >= 0; --i) {
        SimCell& cell = cell_list[i];
        int infra_val = mInfra[cell.index];
        if (infra_val > 0) {
            cell.back_calc_val = std::max(cell.back_calc_val, infra_val);
        }
        if (cell.back_calc_val > 0) {
            int val = cell.back_calc_val;
            for (int p = 0; p < cell.parent_count; ++p) {
                int parent_pos = cell.parents[p];
                cell_list[parent_pos].back_calc_val = std::max(cell_list[parent_pos].back_calc_val, val);
            }
        }
    }

    // Reset thread-local index lookup map for visited cells
    for (const auto& cell : cell_list) {
        cell_list_index_map[cell.index] = -1;
    }

    // Write-back reduction phase under output lock
    std::lock_guard<std::mutex> lock(mOutputMutex);
    for (const auto& cell : cell_list) {
        int idx = cell.index;
        mEnergy[idx] = std::max(mEnergy[idx], cell.z_delta);
        mFlux[idx] = std::max(mFlux[idx], cell.flux);
        mCount[idx]++;
        mEnergySum[idx] += cell.z_delta;
        mFpTravelAngle[idx] = std::max(mFpTravelAngle[idx], cell.max_gamma);
        mSlTravelAngle[idx] = std::max(mSlTravelAngle[idx], cell.sl_gamma);
        mBackCalc[idx] = std::max(mBackCalc[idx], cell.back_calc_val);
    }
}
