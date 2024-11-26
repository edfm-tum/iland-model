#include "understorey.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "modelcontroller.h"
#include "csvfile.h"

#include "understoreyplant.h"
#include "understoreypft.h"

Understorey *Understorey::mInstance = nullptr;

Understorey::Understorey()
{
    mInstance = this;
}

Understorey::~Understorey()
{
    qDeleteAll(mStates);
    mStates.clear();
    mInstance = nullptr;
}

const UnderstoreyState *Understorey::stateById(UStateId id) const
{
    for (int i=0;i<mStates.size();++i)
        if (mStates[i]->id() == id)
            return mStates[i];
    return nullptr;
}



const UnderstoreyCell *Understorey::understoreyCell(QPointF metric_coord) const
{
    // get first the correct RU
    const auto &rugrid = Globals->model()->RUgrid();
    if (!rugrid.coordValid(metric_coord))
        return nullptr;
    const auto ru = rugrid(metric_coord);

    if (!ru) return nullptr;
    Q_ASSERT(ru->index()>=0 && ru->index()<mUnderstoreyRU.size());

    // the mUnderstoreyRU container is a 1:1 copy of the RU container, the index therefore works
    auto &us_ru = mUnderstoreyRU[ru->index()];

    return us_ru.cell(metric_coord);
}

void Understorey::setup()
{
    // load PFTs from external file
    QString path = Globals->path(Globals->settings().value("model.settings.understorey.pftFile"));
    CSVFile pft_file = CSVFile(path);

    if (pft_file.isEmpty())
        throw IException(QString("Understorey: pftFile '%1' does not exist or is empty!").arg(path));

    for (int i = 0; i< pft_file.rowCount(); ++i) {
        mStates.push_back(new UnderstoreyState());
        mStates.back()->setup(UnderstoreySetting(&pft_file, i));
    }

    // setup state transition probabilites, expressions

    // spatial setup per resource unit
    // create the data for understorey as a single chunk of memory
    // by resizing the container
    mUnderstoreyRU.resize(Globals->model()->ruList().size());
    for (int i=0;i<mUnderstoreyRU.size();++i) {
        mUnderstoreyRU[i].setRU(Globals->model()->ruList()[i]);
    }

    Globals->model()->threadExec().run(&UnderstoreyRU::setup, mUnderstoreyRU, false);


}


void Understorey::run()
{
    Globals->model()->threadExec().run(&UnderstoreyRU::calculate, mUnderstoreyRU, false);
}


// ****************** UnderstoreyRU **************************

void UnderstoreyRU::setup()
{
    Q_ASSERT(mRU != nullptr);

    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (auto& cell : mCells) {
        QPointF p = cellCoord(cell);
        // set state of cell to Empty for all valid 10m cells
        if (hg->constValueAt(p).isValid())
            cell.update();
    }
}

void UnderstoreyRU::calculate()
{
    int valid = 0;
    for (auto& cell : mCells) {
        if (cell.isValid()) {
            ++valid;
            cell.mod_plants()[0].setState(valid*(mRU->index()+1));
        }
    }
    qDebug() << "Understorey: valid" << valid;
}

QPointF UnderstoreyRU::cellCoord(int index)
{
    QPointF local( ( (index % cPxPerRU) + 0.5) * cPxSize, ((index/cPxPerRU) + 0.5) * cPxSize );
    return local + mRU->boundingBox().topLeft();
}

const UnderstoreyCell *UnderstoreyRU::cell(QPointF metric_coord) const
{
    // get index_x and index_y relative to the RU corner in 2m resolution
    QPoint p= QPoint(
        (( (int) metric_coord.x()) % cRUSize) / cPxSize,
        (( (int) metric_coord.y()) % cRUSize) / cPxSize
        );
    // get the cell from the RU level container

    int index =  p.y() * cPxPerRU +  p.x() ;
    Q_ASSERT(index>=0 && index<2500);
    return &mCells[index];
}



// ******************************************************************************
UnderstoreyVisualizer *UnderstoreyVisualizer::mVisualizer = nullptr;

UnderstoreyVisualizer::UnderstoreyVisualizer(QObject *parent)
    :QObject(parent)
{
    Q_UNUSED(parent);
}

UnderstoreyVisualizer::~UnderstoreyVisualizer()
{
    GlobalSettings::instance()->controller()->removePaintLayers(mVisualizer);
    mVisualizer = nullptr;
}

void UnderstoreyVisualizer::setupVisualization()
{
    // add agent to UI
    if (mVisualizer)
        delete mVisualizer;

    mVisualizer = new UnderstoreyVisualizer();

    QStringList varlist = {"Understorey - State(1)" };
//    , "Understorey - ShadeTol", // 0,1
//                           "Understorey - TPI", "Understorey - Northness",  // 2,3
//                           "Understorey - Min.Buffer(June)", "Understorey - Min.Buffer(Dec)", // 4,5
//                           "Understorey - Max.Buffer(June)", "Understorey - Max.Buffer(Dec)"};  // 6,7

    QVector<GridViewType> paint_types = {GridViewTurbo};
    //, GridViewTurbo, GridViewTurbo,
    //                                     GridViewTurbo, GridViewTurbo,
    //                                     GridViewTurbo, GridViewTurbo,
    //                                     GridViewTurbo, GridViewTurbo};
    GlobalSettings::instance()->controller()->addPaintLayers(mVisualizer, varlist, paint_types);

}

Grid<double> *UnderstoreyVisualizer::paintGrid(QString what, QStringList &names, QStringList &colors)
{
    Q_UNUSED(names)
    Q_UNUSED(colors)

    if (mGrid.isEmpty()) {
        // setup grid with the dimensions of the iLand LIF grid
        mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(),
                    GlobalSettings::instance()->model()->grid()->cellsize());
        mGrid.wipe(0.);
    }
    int index = 0;
    if (what == "Understorey - State(1)") index = 0;
    if (what == "Understorey - ShadeTol") index = 1;
    if (what == "Understorey - TPI") index = 2;
    if (what == "Understorey - Northness") index = 3;
    if (what == "Understorey - Min.Buffer(June)") index=4;
    if (what == "Understorey - Min.Buffer(Dec)") index=5;
    if (what == "Understorey - Max.Buffer(June)") index=6;
    if (what == "Understorey - Max.Buffer(Dec)") index=7;

    // fill the grid with the expected variable
    const auto &us = Globals->model()->understorey();
    double value=0.;
    for (double *p = mGrid.begin(); p!= mGrid.end(); ++p) {
        QPointF cpp = mGrid.cellCenterPoint(p);
        auto *cell = us->understoreyCell(cpp);
        if (!cell) { *p = 0.; continue; }
        switch (index) {
        case 0: value = cell->plants()[0].state(); break;
        default: value = 0.;
        }

        *p = value;

    }
    /*
    for (auto &ru : GlobalSettings::instance()->model()->ruList()) {
        GridRunner<double> runner(mGrid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            us.
            *gridptr = value;
            ++cell_index;
        }
    } */

/*
    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        const Understorey *clim = ru->Understorey();
        GridRunner<double> runner(mGrid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            switch (index) {
            case 0: value = clim->constCell(cell_index).LAI(); break;
            case 1: value = clim->constCell(cell_index).shadeToleranceMean(); break;
            case 2: value = clim->constCell(cell_index).topographicPositionIndex(); break;
            case 3: value = clim->constCell(cell_index).northness(); break;
                // buffering capacity: minimum summer
            case 4:  value = clim->constCell(cell_index).minimumUnderstoreyBuffering(ru, 5); break;
                // minimum winter
            case 5:  value = clim->constCell(cell_index).minimumUnderstoreyBuffering(ru, 0); break;
                // buffering capacity: max summer
            case 6:  value = clim->constCell(cell_index).maximumUnderstoreyBuffering(ru, 5); break;
                // max winter
            case 7:  value = clim->constCell(cell_index).maximumUnderstoreyBuffering(ru, 0); break;
            }

            *gridptr = value;
            ++cell_index;
        }
    }*/
    return &mGrid;
}

Grid<double> *UnderstoreyVisualizer::grid(QString what)
{
    Grid<double> *grid = new Grid<double>(GlobalSettings::instance()->model()->grid()->metricRect(),
                                          GlobalSettings::instance()->model()->grid()->cellsize());
    grid->wipe(0.);

    int index = -1;
    if (what == "LAI") index = 0;
    if (what == "ShadeTol") index = 1;
    if (what == "TPI") index = 2;
    if (what == "Northness") index = 3;
    if (what == "MinTBuffer") index=4;
    if (what == "MaxTBuffer") index=5;

    if (index < 0)
        throw IException("Understorey: invalid grid name");

    /*
    // fill the grid with the expected variable

    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        const Understorey *clim = ru->Understorey();
        GridRunner<double> runner(grid, ru->boundingBox());
        int cell_index = 0;
        double value;
        while (double *gridptr = runner.next()) {
            switch (index) {
            case 0: value = clim->constCell(cell_index).LAI(); break;
            case 1: value = clim->constCell(cell_index).shadeToleranceMean(); break;
            case 2: value = clim->constCell(cell_index).topographicPositionIndex(); break;
            case 3: value = clim->constCell(cell_index).northness(); break;
            case 4:  value = clim->constCell(cell_index).minimumUnderstoreyBuffering(ru, month); break;
            case 5:  value = clim->constCell(cell_index).maximumUnderstoreyBuffering(ru, month); break;
            }

            *gridptr = value;
            ++cell_index;
        }
    }*/
    return grid;
}
