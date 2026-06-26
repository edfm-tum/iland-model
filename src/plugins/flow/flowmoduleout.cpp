#include "flowmoduleout.h"

#include "flowmodel.h"
#include "grid.h"
#include "model.h"

FlowModuleOut::FlowModuleOut()
{
    setName("Flow (natural hazards) module output", "flow");
    setDescription("Natural hazards. "\
                   "The outputs are created after each year (or spread event) and contain information about spread and damage for the total landscape.\n " \
                   "For spatially explicit outputs, see also the script functions for extracting gridded data.");
    columns() << OutputColumn::year()
              << OutputColumn("type", "flow process type, rockfall/mudflow/avalanche/custom", OutString)
              << OutputColumn("ID", "experiment Id (provided by the user)", OutInteger)
              << OutputColumn("infrastructure", "'true' when results are for infrastructure (backtracking), 'false' otherwise", OutString)
              << OutputColumn("totalStockableCellCount", "total number of stockable cells (a 10m)", OutInteger)
              << OutputColumn("totalInfrastructureCount", "total number of cells (a 10m) with tracks that eventually reach infrastructure (backcasting)", OutInteger)
              << OutputColumn("totalCellCount", "total number of cells (a 10m)", OutInteger)

              << OutputColumn("FSI", "mean forest structure index over all stockable cells (0..1)", OutDouble)
              << OutputColumn("count", "proportion of cells affected (from all starts, incl. starts) over full area", OutDouble)
              << OutputColumn("hitCount", "number of hits (from all starts), mean over full area", OutDouble)
              << OutputColumn("energy", "energy height (m) per px (max of all paths hitting a cell, from all starts), mean over full area", OutDouble)
              << OutputColumn("energySum", "cumulative energy height (m) per px (sum of all paths hitting a cell, from all starts), mean over full area", OutDouble)
              << OutputColumn("flux", "cumulative flux (-) per px (max 1 per cell, from all starts), mean over full area", OutDouble);

}

template <typename TSource, typename TMask>
double maskedAverage(const Grid<TSource> &source, const Grid<TMask> &mask)
{
    // Safety check: ensure both grids have the same size to avoid out-of-bounds access
    if (source.count() != mask.count()) {
        qDebug() << "ERROR: maskedAverage() grid sizes do not match!";
        return 0.;
    }

    int count = 0;
    double value = 0.;

    TSource *src = source.begin();
    TMask *msk = mask.begin();
    TSource *src_end = source.end();

    for (; src != src_end; ++src, ++msk) {
        if (*msk > 0) {
            ++count;
            value += double(*src);
        }
    }

    if (count > 0) return value / double(count);
    return 0.;
}

template <class T>
double proportionNonZero(const Grid<T> &grd) {
    int count = 0;
    for (auto *p = grd.begin(); p!=grd.end(); ++p)
        if (*p > 0) ++count;
    return count / double(grd.count());
}


void FlowModuleOut::exec()
{
    if (!mModel) return;

    *this << GlobalSettings::instance()->currentYear() << mModel->lastFlowType() << mModel->lastExperimentId();
    *this << "false"; // full area
    // stockable 1/0
    int stockable_count = mModel->stockable().avg() * mModel->stockable().count();

    *this << stockable_count << 0 << mModel->stockable().count();
    // calculate cell values
    *this << maskedAverage( mModel->fsi(), mModel->stockable() ); // FSI per stockable area
    // pixel affected, mean hit count
    *this << proportionNonZero(mModel->count()) << mModel->count().avg();
    // energy, energySum, flux
    *this << mModel->energy().avg() << mModel->energySum().avg() << mModel->flux().avg();
    writeRow();

    // If we have infrastructure, we calculate stats also for *only* the backtracking zone
    //
    // proportion of cells
    double infra_prop = proportionNonZero(mModel->backCalc());
    if (infra_prop > 0.) {
        *this << GlobalSettings::instance()->currentYear() << mModel->lastFlowType() << mModel->lastExperimentId();
        *this << "true"; // infrastructure

        *this << 0 << infra_prop*mModel->infra().count() << mModel->infra().count();
        // calculate cell values
        *this << maskedAverage( mModel->fsi(), mModel->backCalc() ); // FSI per stockable area
        // pixel affected, mean hit count
        *this << infra_prop << maskedAverage(mModel->count(), mModel->backCalc());
        // energy, energySum, flux
        *this << maskedAverage(mModel->energy(), mModel->backCalc())
              << maskedAverage(mModel->energySum(), mModel->backCalc())
              << maskedAverage(mModel->flux(), mModel->backCalc());
        writeRow();

    }


}

void FlowModuleOut::setup()
{

}
