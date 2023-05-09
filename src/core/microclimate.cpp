
#include "microclimate.h"

#include "resourceunit.h"
#include "species.h"
#include "tree.h"
#include "modelcontroller.h"


Microclimate::Microclimate(const ResourceUnit *ru)
{
    mRU = ru;
    mCells = new MicroclimateCell[cHeightPerRU*cHeightPerRU];
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

    // loop over trees and calculate aggregate values
    for ( QVector<Tree>::const_iterator t = mRU->constTrees().constBegin(); t != mRU->constTrees().constEnd(); ++t) {
        int idx = cellIndex(t->position());

        ba_total[idx] += t->basalArea();
        lai_total[idx] += t->leafArea();
        if (t->species()->isConiferous())
            ba_conifer[idx] += t->basalArea();
    }

    // now write back to the microclimate store
    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        cell(i).setLAI( lai_total[i] / cHeightPixelArea ); // calculate m2/m2
        cell(i).setConiferShare( ba_total[i] > 0. ? ba_conifer[i] / ba_total[i] : 0.);
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

MicroclimateVisualizer *MicroclimateVisualizer::mVisualizer = nullptr;

MicroclimateVisualizer::MicroclimateVisualizer(QObject *parent)
{
}

MicroclimateVisualizer::~MicroclimateVisualizer()
{
    mVisualizer = nullptr;
}

void MicroclimateVisualizer::setupVisualization()
{
    // add agent to UI
    if (mVisualizer)
        delete mVisualizer;

    mVisualizer = new MicroclimateVisualizer();

    QStringList varlist = {"Microclimate - coniferShare", "Microclimate - LAI"};
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
            }

            *gridptr = value;
            ++cell_index;
        }
    }
    return &mGrid;
}
