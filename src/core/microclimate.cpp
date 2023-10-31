
#include "microclimate.h"

#include "resourceunit.h"
#include "species.h"
#include "tree.h"
#include "modelcontroller.h"
#include "dem.h"
#include "phenology.h"
#include "climate.h"

Microclimate::MicroClimateSettings Microclimate::mSettings = {true, true, true};


Microclimate::Microclimate(const ResourceUnit *ru)
{
    mRU = ru;
    mCells = new MicroclimateCell[cHeightPerRU*cHeightPerRU];
    mIsSetup = false;
    // setup of effect switches (static variable)
    mSettings.barkbeetle_effect = GlobalSettings::instance()->settings().valueBool("model.climate.microclimate.barkbeetle");
    mSettings.decomposition_effect = GlobalSettings::instance()->settings().valueBool("model.climate.microclimate.decomposition");
    mSettings.establishment_effect = GlobalSettings::instance()->settings().valueBool("model.climate.microclimate.establishment");
}

Microclimate::~Microclimate()
{
    delete[] mCells;
}

void Microclimate::calculateVegetation()
{
    QVector<double> ba_total(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> ba_evergreen(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> lai_total(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> shade_tol(cHeightPerRU*cHeightPerRU, 0.);

    if (!mIsSetup)
        // calculate (only once) northness and other factors that only depend on elevation model
        calculateFixedFactors();


    // loop over trees and calculate aggregate values
    for ( QVector<Tree>::const_iterator t = mRU->constTrees().constBegin(); t != mRU->constTrees().constEnd(); ++t) {
        int idx = cellIndex(t->position());

        ba_total[idx] += t->basalArea();
        lai_total[idx] += t->leafArea();
        if (t->species()->isEvergreen())
            ba_evergreen[idx] += t->basalArea();
        // shade-tolerance uses species parameter light response class
        shade_tol[idx] += t->species()->lightResponseClass() * t->basalArea();
    }

    // now write back to the microclimate store
    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        cell(i).setLAI( lai_total[i] / cHeightPixelArea ); // calculate m2/m2
        cell(i).setEvergreenShare( ba_total[i] > 0. ? ba_evergreen[i] / ba_total[i] : 0.);
        cell(i).setShadeToleranceMean( ba_total[i] > 0. ? shade_tol[i] / ba_total[i] : 0.);
    }

    // do additionally calculate and buffer values on RU resolution for performance reasons
    calculateRUMeanValues();

}


void Microclimate::calculateRUMeanValues()
{


    const int pheno_broadleaved = 1;
    const Phenology &pheno = mRU->climate()->phenology(pheno_broadleaved );

    // run over the year
    double buffer_min;
    double buffer_max;
    int n;
    const ClimateDay *cday;
    double mean_tmin[12];
    double mean_tmax[12];

    for (int i=0;i<12;++i) {
        mean_tmin[i] = 0.;
        mean_tmax[i] = 0.;
    }
    // calculate mean min / max temperature
    for (cday = mRU->climate()->begin(); cday != mRU->climate()->end(); ++cday) {
        mean_tmin[cday->month - 1] += cday->min_temperature;
        mean_tmax[cday->month - 1] += cday->max_temperature;
    }
    for (int i=0;i<12;++i) {
        mean_tmin[i] /= mRU->climate()->days(i);
        mean_tmax[i] /= mRU->climate()->days(i);
    }

    // run calculations
    for (int m=0;m<12;++m) {
        // loop over all cells and calculate buffering
        buffer_min=0.;
        buffer_max=0.;
        n=0;
        // calculate gsi, and re-use for each cell of the RU
        double gsi = pheno.monthArray()[m];
        for (int i=0;i < cHeightPerRU*cHeightPerRU; ++i) {
            if (mCells[i].valid()) {

                buffer_min += mCells[i].minimumMicroclimateBuffering(gsi, mean_tmin[m]);
                buffer_max += mCells[i].maximumMicroclimateBuffering(gsi, mean_tmax[m]);
                ++n;
            }
        }

        // calculate mean values for RU and save for later
        buffer_min = n>0 ? buffer_min / n : 0.;
        buffer_max = n>0 ? buffer_max / n : 0.;
        if (abs(buffer_min) > 15 || abs(buffer_max)>15) {
            qDebug() << "Microclimate: dubious buffering: RU: " << mRU->id() << ", buffer_min:" << buffer_min << ", buffermax:" << buffer_max;
            buffer_min = 0.;
            buffer_max = 0.; // just make sure nothing bad happens downstream

        }

        mRUvalues[m] = QPair<float, float>(static_cast<float>(buffer_min),
                                             static_cast<float>(buffer_max));

    }

}

double Microclimate::minimumMicroclimateBufferingRU(int month) const
{
    return mRUvalues[month].first;
}


double Microclimate::maximumMicroclimateBufferingRU(int month) const
{
    return mRUvalues[month].second;
 }

double Microclimate::meanMicroclimateBufferingRU(int month) const
{
    // calculate mean value of min and max buffering
    double buffer = ( minimumMicroclimateBufferingRU(month) +
                     maximumMicroclimateBufferingRU(month) ) / 2.;
    return buffer;
}



int Microclimate::cellIndex(const QPointF &coord)
{
    // convert to index
    QPointF local = coord - mRU->boundingBox().topLeft();
    Q_ASSERT(local.x()>=0 && local.x()<100 && local.y()>=0 && local.y() < 100);

    int index = ( int( local.y() / cHeightPerRU) ) * cHeightPerRU + ( int( local.x() / cHeightPerRU ) );
    return index;
}

QPointF Microclimate::cellCoord(int index)
{
    QPointF local( ( (index % cHeightPerRU) + 0.5) * cHeightPerRU, ((index/cHeightPerRU) + 0.5) * cHeightPerRU );
    return local + mRU->boundingBox().topLeft();
}

void Microclimate::calculateFixedFactors()
{
    if (!GlobalSettings::instance()->model()->dem())
        throw IException("The iLand Microclimate module requires a digital elevation model (DEM).");


    // extract fixed factors from DEM
    const DEM *dem =  GlobalSettings::instance()->model()->dem();

    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();

    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        QPointF p = cellCoord(i);

        double aspect = dem->aspectGrid()->constValueAt(p)*M_PI / 180.;
        double northness = cos(aspect);

        // slope
        double slope = dem->slopeGrid()->constValueAt(p); // percentage of degrees, i.e. 1 = 45 degrees
        slope = atan(slope) * M_PI/180; // convert degree, thanks Kristin for spotting the error in a previous version

        // topographic position
        const int radius = 500;
        double tpi = dem->topographicPositionIndex(p, radius);

        cell(i).setNorthness(northness);
        cell(i).setSlope(slope);
        cell(i).setTopographicPositionIndex(tpi);

        // we only process cells that are stockable
        if (!hg->constValueAt(p).isValid())
            cell(i).setInvalid();

    }

    mIsSetup = true;
}

MicroclimateVisualizer *MicroclimateVisualizer::mVisualizer = nullptr;

MicroclimateVisualizer::MicroclimateVisualizer(QObject *parent)
{
}

MicroclimateVisualizer::~MicroclimateVisualizer()
{
    GlobalSettings::instance()->controller()->removePaintLayers(mVisualizer);
    mVisualizer = nullptr;
}

void MicroclimateVisualizer::setupVisualization()
{
    // add agent to UI
    if (mVisualizer)
        delete mVisualizer;

    mVisualizer = new MicroclimateVisualizer();

    QStringList varlist = {"Microclimate - evergreenShare", "Microclimate - LAI", "Microclimate - ShadeTol", // 0,1,2
                           "Microclimate - TPI", "Microclimate - Northness",  // 3,4
                           "Microclimate - Min.Buffer(June)", "Microclimate - Min.Buffer(Dec)", // 5,6
                           "Microclimate - Max.Buffer(June)", "Microclimate - Max.Buffer(Dec)", "Microclimate - Slope"};  // 7,8,9

    QVector<GridViewType> paint_types = {GridViewTurbo, GridViewTurbo, GridViewTurbo,
                                         GridViewTurbo, GridViewTurbo,
                                         GridViewTurbo, GridViewTurbo,
                                         GridViewTurbo, GridViewTurbo};
    GlobalSettings::instance()->controller()->addPaintLayers(mVisualizer, varlist, paint_types);

}

Grid<double> *MicroclimateVisualizer::paintGrid(QString what, QStringList &names, QStringList &colors)
{
    Q_UNUSED(names)
    Q_UNUSED(colors)

    if (mGrid.isEmpty()) {
        // setup grid with the dimensions of the iLand height grid
        mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(),
                    GlobalSettings::instance()->model()->heightGrid()->cellsize());
        mGrid.wipe(0.);
    }
    int index = 0;
    if (what == "Microclimate - evergreenShare") index = 0;
    if (what == "Microclimate - LAI") index = 1;
    if (what == "Microclimate - ShadeTol") index = 2;
    if (what == "Microclimate - TPI") index = 3;
    if (what == "Microclimate - Northness") index = 4;
    if (what == "Microclimate - Min.Buffer(June)") index=5;
    if (what == "Microclimate - Min.Buffer(Dec)") index=6;
    if (what == "Microclimate - Max.Buffer(June)") index=7;
    if (what == "Microclimate - Max.Buffer(Dec)") index=8;
    if (what == "Microclimate - Slope") index=9;

    // fill the grid with the expected variable

    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        const Microclimate *clim = ru->microClimate();
        GridRunner<double> runner(mGrid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            switch (index) {
            case 0: value = clim->constCell(cell_index).evergreenShare(); break;
            case 1: value = clim->constCell(cell_index).LAI(); break;
            case 2: value = clim->constCell(cell_index).shadeToleranceMean(); break;
            case 3: value = clim->constCell(cell_index).topographicPositionIndex(); break;
            case 4: value = clim->constCell(cell_index).northness(); break;
                // buffering capacity: minimum summer
            case 5:  value = clim->constCell(cell_index).minimumMicroclimateBuffering(ru, 180); break;
                // minimum winter
            case 6:  value = clim->constCell(cell_index).minimumMicroclimateBuffering(ru, 0); break;
                // buffering capacity: max summer
            case 7:  value = clim->constCell(cell_index).maximumMicroclimateBuffering(ru, 180); break;
                // max winter
            case 8:  value = clim->constCell(cell_index).maximumMicroclimateBuffering(ru, 0); break;
            case 9: value = clim->constCell(cell_index).slope(); break;
            }

            *gridptr = value;
            ++cell_index;
        }
    }
    return &mGrid;
}

Grid<double> *MicroclimateVisualizer::grid(QString what, int dayofyear)
{
    Grid<double> *grid = new Grid<double>(GlobalSettings::instance()->model()->heightGrid()->metricRect(),
                                         GlobalSettings::instance()->model()->heightGrid()->cellsize());
    grid->wipe(0.);
    int index = -1;
    if (what == "evergreenShare") index = 0;
    if (what == "LAI") index = 1;
    if (what == "ShadeTol") index = 2;
    if (what == "TPI") index = 3;
    if (what == "Northness") index = 4;
    if (what == "MinTBuffer") index=5;
    if (what == "MaxTBuffer") index=6;
    if (what == "GSI") index=7;
    if (what == "Slope") index=8;

    if (index < 0)
        throw IException("Microclimate: invalid grid name");

    // fill the grid with the expected variable

    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        const Microclimate *clim = ru->microClimate();
        GridRunner<double> runner(grid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            switch (index) {
            case 0: value = clim->constCell(cell_index).evergreenShare(); break;
            case 1: value = clim->constCell(cell_index).LAI(); break;
            case 2: value = clim->constCell(cell_index).shadeToleranceMean(); break;
            case 3: value = clim->constCell(cell_index).topographicPositionIndex(); break;
            case 4: value = clim->constCell(cell_index).northness(); break;
            case 5:  value = clim->constCell(cell_index).minimumMicroclimateBuffering(ru, dayofyear); break;
            case 6:  value = clim->constCell(cell_index).maximumMicroclimateBuffering(ru, dayofyear); break;
            case 7:  value = clim->constCell(cell_index).growingSeasonIndex(ru, dayofyear); break;
            case 8: value = clim->constCell(cell_index).slope(); break;
            }

            *gridptr = value;
            ++cell_index;
        }
    }
    return grid;
}

MicroclimateCell::MicroclimateCell(double evergreen_share, double lai, double shade_tol, double tpi, double northness, double slope)
{
    setEvergreenShare(evergreen_share);
    setLAI(lai);
    setShadeToleranceMean(shade_tol);
    setTopographicPositionIndex(tpi);
    setNorthness(northness);
    setSlope(slope);
}

double MicroclimateCell::growingSeasonIndex(const ResourceUnit *ru, int dayofyear) const
{
    const int pheno_broadleaved = 1;
    const Phenology &pheno = ru->climate()->phenology(pheno_broadleaved );

    int rDay, rMonth;
    ru->climate()->toDate(dayofyear, &rDay, &rMonth);

    double gsi = pheno.monthArray()[rMonth];
    return gsi;
}

double MicroclimateCell::minimumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const
{
    double gsi = growingSeasonIndex(ru, dayofyear);
    double macro_t_min = ru->climate()->dayOfYear(dayofyear)->min_temperature;
    return minimumMicroclimateBuffering(gsi, macro_t_min);
}

double MicroclimateCell::maximumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const
{
    double gsi = growingSeasonIndex(ru, dayofyear);
    double macro_t_max = ru->climate()->dayOfYear(dayofyear)->max_temperature;
    return maximumMicroclimateBuffering(gsi, macro_t_max);
}

double MicroclimateCell::minimumMicroclimateBuffering(double gsi, double macro_t_min) const
{
    // old: "Minimum temperature buffer ~ -1.7157325 - 0.0187969*North + 0.0161997*RelEmin500 + 0.0890564*lai + 0.3414672*stol + 0.8302521*GSI + 0.0208083*prop_evergreen - 0.0107308*GSI:prop_evergreen"
    // Buffer_minT = 0.6077 – 0.0088 * Macroclimate_minT + 0.3548 * Northness  + 0.0872 * Slope + 0.0202 * TPI - 0.0330 * LAI + 0.0502 * STol – 0.7601 * Evergreen – 0.8385 * GSI:Evergreen

    double buf = 0.6077 +
                 -0.0088 *macro_t_min +
                 0.3548*northness() +
                 0.0872*slope() +
                 0.0202*topographicPositionIndex() +
                 -0.0330*LAI() +
                 0.0502*shadeToleranceMean() +
                 -0.7601*evergreenShare() +
                 -0.8385*gsi*evergreenShare();
    if (abs(buf)>10)
        buf=0;
    return buf;

}

double MicroclimateCell::maximumMicroclimateBuffering(double gsi, double macro_t_max) const
{
    // old: "Maximum temperature buffer ~ 1.9058391 - 0.2528409*North - 0.0027037*RelEmin500 - 0.1549061*lai - 0.3806543*stol - 1.2863341*GSI - 0.8070951*prop_evergreen + 0.5004421*GSI:prop_evergreen"
    // Buffer_maxT = 2.7839 – 0.2729 * Macroclimate_maxT - 0.5403 * Northness  - 0.1127 * Slope + 0.0155 * TPI – 0.3182 * LAI + 0.1403 * STol – 1.1039 * Evergreen + 6.9670 * GSI:Evergreen
    double buf = 2.7839 +
                 -0.2729*macro_t_max +
                 -0.5403*northness() +
                 -0.1127*slope() +
                 0.0155*topographicPositionIndex() +
                 -0.3182*LAI() +
                 0.1403*shadeToleranceMean() +
                 -1.1039*evergreenShare() +
                 6.9670*gsi*evergreenShare();

    if (abs(buf)>10)
        buf=0;
    return buf;
}
