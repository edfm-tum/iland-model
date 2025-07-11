#include "understory.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "modelcontroller.h"
#include "csvfile.h"
#include "debugtimer.h"

#include "understoryplant.h"
#include "understorypft.h"

Understory *Understory::mInstance = nullptr;

Understory::Understory()
{
    mInstance = this;
}

Understory::~Understory()
{
    qDeleteAll(mStates);
    mStates.clear();
    mInstance = nullptr;
}

const UnderstoryPFT *Understory::pftByName(const QString &name) const
{
    for (int i=0;i<mPFTs.size();++i)
        if (mPFTs[i]->name() == name)
            return mPFTs[i];
    return nullptr;

}

const UnderstoryState *Understory::stateById(UStateId id) const
{
    for (int i=0;i<mStates.size();++i)
        if (mStates[i]->id() == id)
            return mStates[i];
    return nullptr;
}



const UnderstoryCell *Understory::understoryCell(QPointF metric_coord) const
{
    auto us_ru = understoryRU(metric_coord);
    if (!us_ru)
        return nullptr;

    return us_ru->cell(metric_coord);
}

const UnderstoryRU *Understory::understoryRU(QPointF metric_coord) const
{
    // get first the correct RU
    const auto &rugrid = Globals->model()->RUgrid();
    if (!rugrid.coordValid(metric_coord))
        return nullptr;
    const auto ru = rugrid(metric_coord);

    if (!ru) return nullptr;
    Q_ASSERT(ru->index()>=0 && ru->index()<mUnderstoryRU.size());

    // the mUnderstoryRU container is a 1:1 copy of the RU container, the index therefore works
    auto &us_ru = mUnderstoryRU[ru->index()];
    return &us_ru;
}

void Understory::setup()
{
    DebugTimer t("Understory - setup");
    qDebug() << "Understory module - setup";
    // load PFTs from external file
    QString path = Globals->path(Globals->settings().value("model.settings.understory.pftFile"));
    CSVFile pft_file = CSVFile(path);

    if (pft_file.isEmpty())
        throw IException(QString("Understory: pftFile '%1' does not exist or is empty!").arg(path));

    for (int i = 0; i< pft_file.rowCount(); ++i) {
        mPFTs.push_back(new UnderstoryPFT());
        mPFTs.back()->setup(UnderstorySetting(&pft_file, i), mPFTs.size()-1);
    }
    qDebug() << mPFTs.size() << "PFTs loaded from" << path;

    // load PFTs from external file
    path = Globals->path(Globals->settings().value("model.settings.understory.statesFile"));
    CSVFile states_file = CSVFile(path);

    if (states_file.isEmpty())
        throw IException(QString("Understory: statesFile '%1' does not exist or is empty!").arg(path));
    for (int i = 0; i< states_file.rowCount(); ++i) {
        mStates.push_back(new UnderstoryState());
        mStates.back()->setup(UnderstorySetting(&states_file,i), mStates.size()-1);
    }
    qDebug() << mStates.size() << "understory states loaded from" << path;

    // check for consistency
    checkStateSequence();


    // spatial setup per resource unit
    // create the data for understory as a single chunk of memory
    // by resizing the container
    mUnderstoryRU.resize(Globals->model()->ruList().size());
    for (int i=0;i<mUnderstoryRU.size();++i) {
        mUnderstoryRU[i].setRU(Globals->model()->ruList()[i]);
    }

    Globals->model()->threadExec().run(&UnderstoryRU::setup, mUnderstoryRU, false);

    qDebug() << "Understory module setup complete.";

}


void Understory::run()
{
    // run the growth for all resource units
    DebugTimer t1("Understory - grow");
    Globals->model()->threadExec().run(&UnderstoryRU::growth, mUnderstoryRU, true);

    // run the establishment routine for understory for all resource units
    DebugTimer t2("Understory - establishment");
    Globals->model()->threadExec().run(&UnderstoryRU::establishment, mUnderstoryRU, true);


}

void Understory::checkStateSequence()
{
    for (auto &pft : mPFTs) {
        int min_index = -1;
        int max_index = -1;
        int size_class  = -1;
        bool in = false; bool out = false;
        for (auto &state : mStates) {
            if (out && state->pftIndex() == pft->index())
                throw IException(QString("Understory: Invalid state sequence! found '%1' is outside its group!").arg(state->name()));

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
                throw IException("Understory: 'size' attribute of PFT '%1' are not strictly increasing in 'statesFile'!");
            size_class = state->sizeClass();

        }
        if (min_index <0 || max_index < 0)
            throw IException(QString("Understory: PFT '%1' has no states in statesFile! *Every* PFT needs them.").arg(pft->name()));
        mStates[min_index]->setFirstState();
        pft->setFirstState(mStates[min_index]->id());
        mStates[max_index]->setFinalState();
        int n_states = max_index - min_index + 1;
        pft->setNumberOfStates(n_states);

        qDebug() << "PFT: " << pft->name() << "First/Last:" << mStates[min_index]->name() << ".." << mStates[max_index]->name();
    }
}




// ******************************************************************************
UnderstoryVisualizer *UnderstoryVisualizer::mVisualizer = nullptr;
QStringList UnderstoryVisualizer::mVarList = {};

UnderstoryVisualizer::UnderstoryVisualizer(QObject *parent)
    :QObject(parent)
{
    Q_UNUSED(parent);
}

UnderstoryVisualizer::~UnderstoryVisualizer()
{
    GlobalSettings::instance()->controller()->removePaintLayers(mVisualizer);
    mVisualizer = nullptr;
}

void UnderstoryVisualizer::setupVisualization()
{
    // add agent to UI
    if (mVisualizer)
        delete mVisualizer;

    mVisualizer = new UnderstoryVisualizer();

    mVarList = {"Understory - CellsOccupied", // 0
                "Understory - SlotsOccupied", // 1
                "Understory - Biomass", "Understory - LAI", "Understory - maxHeight", // 2,3,4
                // RU-LEVEL
                "Understory - RU Covered", // 0
                "Understory - RU SlotsOccupied", // 1
                "Understory - RU LAI", // 2
                "Understory - RU Biomass", // 3
    };
    QStringList var_desc = {
                "Number of 'plants' on cell",
                "Number of 'slots' occupied per cell",
                "Total biomass (kg/m2?)",
                "LAI (m2/m2) of understory",
                "Maximum height (m) on cell",
                // RU - LEVEL
                "Percent of RU area with >0 plants (%)",
                "Percent of total #slots of RU occupied (%)",
                "Total LAI on RU (m2/m2)",
                "Total biomass on RU (kg/ha?)"
    };

    const auto &us = Globals->model()->understory();
    QStringList pft_filter = { "(none)"};
    for (auto *p : us->PFTs())
        pft_filter.push_back(p->name());

    QString filter_str = QString("Understory - Filter - %1").arg(pft_filter.join(","));
    mVarList.push_back(filter_str);
    var_desc.push_back(" (filter) ");

    QVector<GridViewType> paint_types = {GridViewTurbo,
                                         GridViewTurbo,
                                         GridViewTurbo,GridViewTurbo,GridViewTurbo,
                                         GridViewTurbo}; // last one for filter

    GlobalSettings::instance()->controller()->addPaintLayers(mVisualizer, mVarList, paint_types, var_desc);

}

Grid<double> *UnderstoryVisualizer::paintGrid(QString what, QStringList &names, QStringList &colors)
{
    Q_UNUSED(names)
    Q_UNUSED(colors)

    if (mGrid.isEmpty()) {
        // setup grid with the dimensions of the iLand LIF grid
        mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(),
                    GlobalSettings::instance()->model()->grid()->cellsize());
        mGrid.wipe(0.);

        mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                      GlobalSettings::instance()->model()->RUgrid().cellsize());
        mRUGrid.wipe(0.);
    }
    int index = mVarList.indexOf(what);

    // fill the grid with the expected variable
    const auto &us = Globals->model()->understory();
    double value=0.;
    constexpr int min_idx_ru = 5;
    if (index < min_idx_ru) {
        // 2m grid
        for (double *p = mGrid.begin(); p!= mGrid.end(); ++p) {
            QPointF cpp = mGrid.cellCenterPoint(p);
            auto *cell = us->understoryCell(cpp);
            if (!cell) { *p = 0.; continue; }
            auto cell_stats = cell->stats(mPFTFilter);
            switch (index) {
            case 0: value = cell_stats.cellsOccupied; break;
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
            const auto us_ru = us->understoryRU(mRUGrid.cellCenterPoint(p));
            if (us_ru) {
                auto &stats = us_ru->stats();
                const UnderstoryStatsCell *s = &stats.ru_stats;
                if (mPFTFilter) {
                    s = &us_ru->pftStats()[mPFTFilter->index()].ru_stats;
                }

                switch (index - min_idx_ru) {
                case 0: *p = s->cellsOccupied; break;
                case 1: *p = s->slotsOccupied; break;
                case 2: *p = s->LAI; break;
                case 3: *p = s->biomass; break;
                }
            }
        }
        return &mRUGrid;
    }
}

void UnderstoryVisualizer::filterChanged(int filter_index)
{
    mPFTFilter = nullptr;
    const auto &us = Globals->model()->understory();
    int index = 1; // skip the first element (the "(none)" in the filter)
    for (auto *p : us->PFTs()) {
        if (index == filter_index) {
            mPFTFilter = p;
        }
        ++index;
    }
    qDebug() << "filter changed" << filter_index << "pft:" << (mPFTFilter ? mPFTFilter->name() : "null");


}

Grid<double> *UnderstoryVisualizer::grid(QString what)
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
        throw IException("Understory: invalid grid name");


    return grid;
}
