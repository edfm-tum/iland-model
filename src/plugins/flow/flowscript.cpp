#include "flowscript.h"
#include "scriptglobal.h"
#include "flowmodule.h"
#include "globalsettings.h"
#include "model.h"
#include "scriptgrid.h"

FlowScript::FlowScript(QObject *)
{
    mModule = nullptr;
}


void FlowScript::setStartPoint(double x, double y)
{
    if (!mModule) return;
    try {
        mModule->mFlow.setStartArea(QPointF(x, y));
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::setStartRectangle(double x1, double y1, double x2, double y2)
{
    if (!mModule) return;
    try {
        QRectF rect(QPointF(x1, y1), QPointF(x2, y2));
        mModule->mFlow.setStartArea(rect);
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::setStartPolygon(int standId, ScriptGrid *grid)
{
    if (!mModule) return;
    try {
        mModule->mFlow.setStartArea(standId, grid ? grid->grid() : nullptr);
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::setInfrastructure(ScriptGrid *grid)
{
    if (!mModule) return;
    try {
        if (!grid || !grid->grid())
            ScriptGlobal::throwError("FlowScript:setInfrastructure: empty or missing grid!");
        Grid<int> int_grid;
        int_grid.setup(grid->grid()->metricRect(), grid->grid()->cellsize());
        for (int i=0;i<int_grid.count();++i)
            int_grid[i] = (*grid->grid())[i];

        mModule->mFlow.setInfrastructure(int_grid);
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }

}

void FlowScript::setFSI(ScriptGrid *grid)
{
    if (!mModule) return;
    try {
        if (!grid || !grid->grid()) {
            // Reset to dynamic FSI calculations
            mModule->mCustomFSISet = false;
            return;
        }
        
        if (grid->grid()->count() != mModule->mFSI.count()) {
            ScriptGlobal::throwError("FlowScript:setFSI: Grid size mismatch! The FSI grid must match the project landscape size.");
        }

        // Copy grid values from double (ScriptGrid) to float (mFSI)
        for (int i = 0; i < mModule->mFSI.count(); ++i) {
            mModule->mFSI[i] = float((*grid->grid())[i]);
        }
        mModule->mCustomFSISet = true;
        qDebug() << "FlowScript::setFSI: custom FSI grid successfully copied. Cells count:" 
                 << mModule->mFSI.count() << ", average value:" << grid->grid()->avg();
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::setForestEffect(bool enabled)
{
    if (!mModule) return;
    mModule->mForestEffectEnabled = enabled;
}

void FlowScript::setCustomLandscape(ScriptGrid *grid)
{
    if (!mModule) return;
    try {
        if (!grid || !grid->grid()) {
            ScriptGlobal::throwError("FlowScript:setCustomLandscape: empty or missing grid!");
        }

        if (grid->grid()->cellsize() != 10) {
            ScriptGlobal::throwError("FlowScript:setCustomLandscape: Grid resolution must be exactly 10m!");
        }

        // Setup the internal grids of FlowModule to match the custom DEM
        mModule->mGrid.setup(grid->grid()->metricRect(), grid->grid()->cellsize());
        mModule->mFSI.setup(grid->grid()->metricRect(), grid->grid()->cellsize());

        // Re-setup the layered grid (visualizer) to use the new size
        mModule->mLayers.setGrid(mModule->mGrid);

        // Convert double (ScriptGrid) to float (mDEM copy in FlowModel)
        Grid<float> dem_float;
        dem_float.setup(grid->grid()->metricRect(), grid->grid()->cellsize());
        for (int i = 0; i < dem_float.count(); ++i) {
            dem_float[i] = float((*grid->grid())[i]);
        }

        mModule->mFlow.setup(&dem_float, mModule->mFSI);

        mModule->mCustomLandscapeSet = true;

        qDebug() << "FlowScript::setCustomLandscape: Custom landscape successfully set."
                 << "Metric rect:" << grid->grid()->metricRect()
                 << "Cells count:" << grid->grid()->count();

    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::reloadParameters()
{
    if (!mModule) return;
    try {
        mModule->loadParameters();
        qDebug() << "Flow parameters reloaded from XML.";
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::setStartStand(int standId)
{
    if (!mModule) return;
    try {
        mModule->mFlow.setStartArea(standId);
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

void FlowScript::run(QString type, int experimentID)
{
    if (!mModule) return;
    try {
        mModule->runFlow(type, experimentID);
    } catch (const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

QJSValue FlowScript::grid(QString type)
{
    if (!mModule) return QJSValue();
    int idx = mModule->mLayers.indexOf(type);
    if (idx<0)
        qDebug() << "ERROR: FlowScript:grid(): invalid grid" << type;
    // this is a copy
    Grid<double> *value_grid = mModule->mLayers.copyGrid(idx);

    QJSValue g = ScriptGrid::createGrid(value_grid, type);
    return g;
}
