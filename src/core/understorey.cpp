#include "understorey.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "modelcontroller.h"
#include "csvfile.h"
#include "debugtimer.h"

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

const UnderstoreyPFT *Understorey::pftByName(const QString &name) const
{
    for (int i=0;i<mPFTs.size();++i)
        if (mPFTs[i]->name() == name)
            return mPFTs[i];
    return nullptr;

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
    auto us_ru = understoreyRU(metric_coord);
    if (!us_ru)
        return nullptr;

    return us_ru->cell(metric_coord);
}

const UnderstoreyRU *Understorey::understoreyRU(QPointF metric_coord) const
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
    return &us_ru;
}

void Understorey::setup()
{
    DebugTimer t("Understorey - setup");
    qDebug() << "Understorey module - setup";
    // load PFTs from external file
    QString path = Globals->path(Globals->settings().value("model.settings.understorey.pftFile"));
    CSVFile pft_file = CSVFile(path);

    if (pft_file.isEmpty())
        throw IException(QString("Understorey: pftFile '%1' does not exist or is empty!").arg(path));

    for (int i = 0; i< pft_file.rowCount(); ++i) {
        mPFTs.push_back(new UnderstoreyPFT());
        mPFTs.back()->setup(UnderstoreySetting(&pft_file, i), mPFTs.size()-1);
    }
    qDebug() << mPFTs.size() << "PFTs loaded from" << path;

    // load PFTs from external file
    path = Globals->path(Globals->settings().value("model.settings.understorey.statesFile"));
    CSVFile states_file = CSVFile(path);

    if (states_file.isEmpty())
        throw IException(QString("Understorey: statesFile '%1' does not exist or is empty!").arg(path));
    for (int i = 0; i< states_file.rowCount(); ++i) {
        mStates.push_back(new UnderstoreyState());
        mStates.back()->setup(UnderstoreySetting(&states_file,i), mStates.size()-1);
    }
    qDebug() << mStates.size() << "understorey states loaded from" << path;

    // check for consistency
    checkStateSequence();


    // spatial setup per resource unit
    // create the data for understorey as a single chunk of memory
    // by resizing the container
    mUnderstoreyRU.resize(Globals->model()->ruList().size());
    for (int i=0;i<mUnderstoreyRU.size();++i) {
        mUnderstoreyRU[i].setRU(Globals->model()->ruList()[i]);
    }

    Globals->model()->threadExec().run(&UnderstoreyRU::setup, mUnderstoreyRU, false);

    qDebug() << "Understorey module setup complete.";

}


void Understorey::run()
{
    // run the growth for all resource units
    DebugTimer t1("Understory - grow");
    Globals->model()->threadExec().run(&UnderstoreyRU::growth, mUnderstoreyRU, true);

    // run the establishment routine for understorey for all resource units
    DebugTimer t2("Understory - establishment");
    Globals->model()->threadExec().run(&UnderstoreyRU::establishment, mUnderstoreyRU, true);


}

void Understorey::checkStateSequence()
{
    for (auto &pft : mPFTs) {
        int min_index = -1;
        int max_index = -1;
        int size_class  = -1;
        bool in = false; bool out = false;
        for (auto &state : mStates) {
            if (out && state->pftIndex() == pft->index())
                throw IException(QString("Understorey: Invalid state sequence! found '%1' is outside its group!").arg(state->name()));

            if (in && state->pftIndex() != pft->index()) {
                out = true; // switched to a different pft
                in = false;
            }
            if (state->pftIndex() != pft->index())
                continue;

            if (!in) {
                // first state of the pft
                min_index = state->id();
                in = true;
                size_class = state->sizeClass();
            }

            max_index = state->id();
            if (state->sizeClass() < size_class)
                throw IException("Understorey: 'size' attribute of PFT '%1' are not strictly increasing in 'statesFile'!");
            size_class = state->sizeClass();

        }
        if (min_index <0 || max_index < 0)
            throw IException(QString("Understorey: PFT '%1' has no states in statesFile! *Every* PFT needs them.").arg(pft->name()));
        mStates[min_index]->setFirstState();
        pft->setFirstState(mStates[min_index]->id());
        mStates[max_index]->setFinalState();

        qDebug() << "PFT: " << pft->name() << "First/Last:" << mStates[min_index]->name() << ".." << mStates[max_index]->name();
    }
}




// ******************************************************************************
UnderstoreyVisualizer *UnderstoreyVisualizer::mVisualizer = nullptr;
QStringList UnderstoreyVisualizer::mVarList = {};

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

    mVarList = {"Understorey - State", // 0
                "Understorey - SlotsOccupied", // 1
                "Understorey - Biomass", "Understorey - LAI", "Understorey - maxHeight", // 2,3,4
                // RU-LEVEL
                "Understorey - RU Covered", // 0
                "Understorey - RU SlotsOccupied", // 1
                "Understorey - RU LAI", // 2
                "Understorey - RU Biomass" // 3
    };
    QStringList var_desc = {
                "state",
                "Number of 'slots' occupied per cell",
                "Total biomass (kg/m2??)",
                "LAI (m2/m2) of understorey",
                "Maximum height (m) on cell",
                // RU - LEVEL
                "Percent of RU area with >0 plants (%)",
                "Percent of total #slots of RU occupied (%)",
                "Total LAI on RU (m2/m2)",
                "Total biomass on RU (kg/ha??)"
    };

    QVector<GridViewType> paint_types = {GridViewTurbo,
                                         GridViewTurbo,
                                         GridViewTurbo,GridViewTurbo,GridViewTurbo};

    GlobalSettings::instance()->controller()->addPaintLayers(mVisualizer, mVarList, paint_types, var_desc);

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

        mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                      GlobalSettings::instance()->model()->grid()->cellsize());
        mRUGrid.wipe(0.);
    }
    int index = mVarList.indexOf(what);

    // fill the grid with the expected variable
    const auto &us = Globals->model()->understorey();
    double value=0.;
    constexpr int min_idx_ru = 5;
    if (index < min_idx_ru) {
        // 2m grid
        for (double *p = mGrid.begin(); p!= mGrid.end(); ++p) {
            QPointF cpp = mGrid.cellCenterPoint(p);
            auto *cell = us->understoreyCell(cpp);
            if (!cell) { *p = 0.; continue; }
            auto cell_stats = cell->stats();
            switch (index) {
            case 0: value = cell->plants()[0].stateId() == std::numeric_limits<UStateId>::max() ? -1 : cell->plants()[0].stateId(); break;
            case 1: value = cell_stats.slotsOccupied; break;
            case 2: value = cell_stats.biomass; break;
            case 3: value = cell_stats.LAI; break;
            case 4: value = cell_stats.height; break;
            default: value = 0.;
            }

            *p = value;

        }
        return &mGrid;
    } else {
        // 100m grid
        for (double *p = mRUGrid.begin(); p!= mRUGrid.end(); ++p) {
            const auto us_ru = us->understoreyRU(mRUGrid.cellCenterPoint(p));
            if (us_ru) {
                auto &stats = us_ru->stats();
                switch (index - min_idx_ru) {
                case 0: *p = stats.ru_stats.NStates; break;
                case 1: *p = stats.ru_stats.slotsOccupied; break;
                case 2: *p = stats.ru_stats.LAI; break;
                case 3: *p = stats.ru_stats.biomass; break;
                }
            }
        }
        return &mRUGrid;
    }
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
