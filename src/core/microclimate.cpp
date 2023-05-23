
#include "microclimate.h"

#include "resourceunit.h"
#include "species.h"
#include "tree.h"
#include "modelcontroller.h"
#include "dem.h"
#include "phenology.h"
#include "climate.h"


Microclimate::Microclimate(const ResourceUnit *ru)
{
    mRU = ru;
    mCells = new MicroclimateCell[cHeightPerRU*cHeightPerRU];
    mIsSetup = false;

}

Microclimate::~Microclimate()
{
    delete[] mCells;
}

void Microclimate::calculateVegetation()
{
    QVector<double> ba_total(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> ba_conifer(cHeightPerRU*cHeightPerRU, 0.);
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
        if (t->species()->isConiferous())
            ba_conifer[idx] += t->basalArea();
        // shade-tolerance uses species parameter light response class
        shade_tol[idx] += t->species()->lightResponseClass() * t->basalArea();
    }

    // now write back to the microclimate store
    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        cell(i).setLAI( lai_total[i] / cHeightPixelArea ); // calculate m2/m2
        cell(i).setConiferShare( ba_total[i] > 0. ? ba_conifer[i] / ba_total[i] : 0.);
        cell(i).setShadeToleranceMean( ba_total[i] > 0. ? shade_tol[i] / ba_total[i] : 0.);
    }

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

    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        QPointF p = cellCoord(i);

        double aspect = dem->aspectGrid()->constValueAt(p)*M_PI / 180.;
        double northness = cos(aspect);
        const int radius = 500;

        double tpi = dem->topographicPositionIndex(p, radius);

        cell(i).setNorthness(northness);
        cell(i).setTopographicPositionIndex(tpi);

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

    QStringList varlist = {"Microclimate - coniferShare", "Microclimate - LAI", "Microclimate - ShadeTol",
                           "Microclimate - TPI", "Microclimate - Northness",
                           "Microclimate - Min.Buffer(June)", "Microclimate - Min.Buffer(Dec)",
                           "Microclimate - Max.Buffer(June)", "Microclimate - Max.Buffer(Dec)"};
    QVector<GridViewType> paint_types = {GridViewTurbo, GridViewTurbo};
    GlobalSettings::instance()->controller()->addPaintLayers(mVisualizer, varlist, paint_types);

}

Grid<double> *MicroclimateVisualizer::paintGrid(QString what, QStringList &names, QStringList &colors)
{
    if (mGrid.isEmpty()) {
        // setup grid with the dimensions of the iLand height grid
        mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(),
                    GlobalSettings::instance()->model()->heightGrid()->cellsize());
        mGrid.wipe(0.);
    }
    int index = 0;
    if (what == "Microclimate - coniferShare") index = 0;
    if (what == "Microclimate - LAI") index = 1;
    if (what == "Microclimate - ShadeTol") index = 2;
    if (what == "Microclimate - TPI") index = 3;
    if (what == "Microclimate - Northness") index = 4;
    if (what == "Microclimate - Min.Buffer(June)") index=5;
    if (what == "Microclimate - Min.Buffer(Dec)") index=6;
    if (what == "Microclimate - Max.Buffer(June)") index=7;
    if (what == "Microclimate - Max.Buffer(Dec)") index=8;

    // fill the grid with the expected variable

    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        const Microclimate *clim = ru->microClimate();
        GridRunner<double> runner(mGrid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            switch (index) {
            case 0: value = clim->constCell(cell_index).coniferShare(); break;
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
            }

            *gridptr = value;
            ++cell_index;
        }
    }
    return &mGrid;
}

double MicroclimateCell::minimumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const
{
    const int pheno_broadleaved = 1;
    const Phenology &pheno = ru->climate()->phenology(pheno_broadleaved );

    int rDay, rMonth;
    ru->climate()->toDate(dayofyear, &rDay, &rMonth);

    double gsi = pheno.month()[rMonth];

    // "Minimum temperature buffer ~ -1.7157325 - 0.0187969*North + 0.0161997*RelEmin500 + 0.0890564*lai + 0.3414672*stol + 0.8302521*GSI + 0.0208083*prop_evergreen - 0.0107308*GSI:prop_evergreen"
    double buf = -1.7157325 +
                 -0.0187969*northness() +
                 0.0161997*topographicPositionIndex() +
                 0.0890564*LAI() +
                 0.3414672*shadeToleranceMean() +
                 0.8302521*gsi +
                 0.0208083*coniferShare() +
                 -0.0107308*gsi*coniferShare();

    return buf;
}

double MicroclimateCell::maximumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const
{
    const int pheno_broadleaved = 1;
    const Phenology &pheno = ru->climate()->phenology(pheno_broadleaved );

    int rDay, rMonth;
    ru->climate()->toDate(dayofyear, &rDay, &rMonth);

    double gsi = pheno.month()[rMonth];

    // "Maximum temperature buffer ~ 1.9058391 - 0.2528409*North - 0.0027037*RelEmin500 - 0.1549061*lai - 0.3806543*stol - 1.2863341*GSI - 0.8070951*prop_evergreen + 0.5004421*GSI:prop_evergreen"
    double buf = 1.9058391 +
                 -0.2528409*northness() +
                 -0.0027037*topographicPositionIndex() +
                 -0.1549061*LAI() +
                 -0.3806543*shadeToleranceMean() +
                 -1.2863341*gsi +
                 -0.8070951*coniferShare() +
                 0.5004421*gsi*coniferShare();

    return buf;
}
