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
