/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#include "scriptgrid.h"
#include "globalsettings.h"
#include "helper.h"
#include "expression.h"
#include "modelcontroller.h"
#include "mapgrid.h"
#include "gisgrid.h"
#include "expressionwrapper.h"
#include "model.h"
#include "tree.h"
#include "scriptglobal.h"


#include <QJSEngine>
#include <QJSValueIterator>

int ScriptGrid::mDeleted = 0;
int ScriptGrid::mCreated = 0;

ScriptGrid::ScriptGrid(QObject *parent) : QObject(parent)
{
    mGrid = nullptr;
    mVariableName = "x"; // default name
    mCreated++;
    mOwner = true;
}

ScriptGrid::~ScriptGrid()
{
    GlobalSettings::instance()->controller()->removeMapGrid(mGrid, nullptr);
    if (mGrid && mOwner) {
        delete mGrid;
    }
    mDeleted++;
    // remove potential links in UI to this grid
    qDebug() << "ScriptGrid::balance: created:" << mCreated << "deleted:" << mDeleted;
}

// create a ScriptGrid-Wrapper around "grid". Note: destructing the 'grid' is done via the JS-garbage-collector.
QJSValue ScriptGrid::createGrid(Grid<double> *grid, QString name)
{
    ScriptGrid *g = new ScriptGrid(grid);
    if (!name.isEmpty())
        g->setName(name);
    QJSValue jsgrid = GlobalSettings::instance()->scriptEngine()->newQObject(g);
    return jsgrid;
}

void ScriptGrid::addToScriptEngine(QJSEngine *engine)
{
    qRegisterMetaType<ScriptGrid*>("ScriptGrid*"); // register type, required to have that type as property
    // allow for "new Grid"
    QJSValue jsMetaObject = engine->newQMetaObject(&ScriptGrid::staticMetaObject);
    engine->globalObject().setProperty("Grid", jsMetaObject);

}

bool ScriptGrid::create(int awidth, int aheight, int acellsize)
{
    if (awidth <= 0 || aheight <= 0 || acellsize <= 0) {
        ScriptGlobal::throwError(QString("ScriptGrid::create: invalid dimensions or cellsize (width: %1, height: %2, cellsize: %3).").arg(awidth).arg(aheight).arg(acellsize));
        return false;
    }
    if (mGrid && mOwner)
        delete mGrid;
    mGrid = new Grid<double>(acellsize, awidth, aheight);
    mGrid->wipe();
    mOwner = true;
    return true;
}

void ScriptGrid::setOrigin(double x, double y)
{
    if (!mGrid) {
        ScriptGlobal::throwError("ScriptGrid::setOrigin: grid not valid!");
        return;
    }
    QRectF rect = mGrid->metricRect();
    rect.moveTo(x,y);
    mGrid->setMetricRect( rect );
}

QJSValue ScriptGrid::copy()
{
    if (!mGrid) {
        ScriptGlobal::throwError("ScriptGrid::copy: grid not valid!");
        return QJSValue();
    }

    ScriptGrid *newgrid = new ScriptGrid();
    // copy the data
    Grid<double> *copy_grid = new Grid<double>(*mGrid);
    newgrid->setGrid(copy_grid);

    QJSValue jsgrid = GlobalSettings::instance()->scriptEngine()->newQObject(newgrid);
    return jsgrid;
}

void ScriptGrid::clear()
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::clear: grid not valid or empty!");
        return;
    }
    mGrid->wipe();
}

void ScriptGrid::paint(double min_val, double max_val)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::paint: grid not valid or empty!");
        return;
    }
    if (GlobalSettings::instance()->controller())
        GlobalSettings::instance()->controller()->paintGrid(mGrid, mVariableName, GridViewRainbow, min_val, max_val);
}

void ScriptGrid::registerUI(QString name)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::registerUI: grid not valid or empty!");
        return;
    }
    if (GlobalSettings::instance()->controller())
        GlobalSettings::instance()->controller()->addScriptLayer(mGrid, nullptr, name.isEmpty() ? mVariableName : name);

}

QString ScriptGrid::info()
{
    if (!mGrid || mGrid->isEmpty())
        return QString("not valid / empty.");
    return QString("grid-dimensions: %1/%2 (cellsize: %5, N cells: %3), grid-name='%4'").arg(mGrid->sizeX()).arg(mGrid->sizeY()).arg(mGrid->count()).arg(mVariableName).arg(mGrid->cellsize());
}

void ScriptGrid::save(QString fileName)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::save: grid not valid or empty!");
        return;
    }
    try {
        fileName = GlobalSettings::instance()->path(fileName);
        gridToFile<double>(*mGrid, fileName);
        qDebug() << "saved grid " << name() << " to " << fileName;
    } catch(const IException &e) {
        ScriptGlobal::throwError(e.message());
    } catch(const std::exception &e) {
        ScriptGlobal::throwError(e.what());
    }
}

bool ScriptGrid::load(QString fileName)
{
    try {
        QString fullPath = GlobalSettings::instance()->path(fileName);
        if (!QFile::exists(fullPath)) {
            QString err = QString("ScriptGrid::load: file '%1' does not exist (resolved path: '%2').").arg(fileName).arg(fullPath);
            ScriptGlobal::throwError(err);
            return false;
        }

        Grid<double> *newGrid = new Grid<double>();
        if (!newGrid->loadGridFromFile(fullPath)) {
            delete newGrid;
            QString err = QString("ScriptGrid::load: loading of grid from '%1' failed.").arg(fileName);
            ScriptGlobal::throwError(err);
            return false;
        }
        if (newGrid->isEmpty()) {
            delete newGrid;
            QString err = QString("ScriptGrid::load: loaded grid '%1' is empty.").arg(fileName);
            ScriptGlobal::throwError(err);
            return false;
        }

        QRectF rect = newGrid->metricRect();
        double wx = GlobalSettings::instance()->settings().valueDouble("model.world.location.x");
        double wy = GlobalSettings::instance()->settings().valueDouble("model.world.location.y");
        // move the grid to match the origin (0/0) of the model
        rect.moveTo( rect.left() - wx, rect.top() - wy );
        newGrid->setMetricRect(rect);

        if (mGrid && mOwner) {
            delete mGrid;
        }
        mGrid = newGrid;
        mOwner = true;
        mVariableName = QFileInfo(fileName).baseName();
        return true;
    } catch (const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::load: error loading '%1': %2").arg(fileName).arg(e.message()));
        return false;
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::load: error loading '%1': %2").arg(fileName).arg(e.what()));
        return false;
    } catch (...) {
        ScriptGlobal::throwError(QString("ScriptGrid::load: unknown error loading '%1'.").arg(fileName));
        return false;
    }
}

void ScriptGrid::apply(QString expression)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::apply: grid not valid or empty!");
        return;
    }

    try {
        Expression expr;
        double *varptr = expr.addVar(mVariableName);
        expr.setExpression(expression);
        expr.parse();

        // now apply function on grid
        for (double *p = mGrid->begin(); p!=mGrid->end(); ++p) {
            *varptr = *p;
            *p = expr.execute();
        }
    } catch(const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::apply: error with expression '%1': %2").arg(expression).arg(e.message()));
    } catch(const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::apply: error with expression '%1': %2").arg(expression).arg(e.what()));
    }
}

void ScriptGrid::combine(QString expression, QJSValue grid_object)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::combine: grid not valid or empty!");
        return;
    }
    if (!grid_object.isObject()) {
        ScriptGlobal::throwError(QString("ScriptGrid::combine: no valid grids object: %1").arg(grid_object.toString()));
        return;
    }
    QVector< Grid<double>* > grids;
    QStringList names;
    QJSValueIterator it(grid_object);
    while (it.hasNext()) {
        it.next();
        names.push_back(it.name());
        QObject *o = it.value().toQObject();
        if (o && qobject_cast<ScriptGrid*>(o)) {
            grids.push_back(qobject_cast<ScriptGrid*>(o)->grid());
            if (!grids.last() || grids.last()->isEmpty() || grids.last()->cellsize() != mGrid->cellsize() || grids.last()->rectangle()!=mGrid->rectangle()) {
                ScriptGlobal::throwError(QString("ScriptGrid::combine: the grid '%1' is empty or has different dimensions: %2")
                                             .arg(it.name()).arg(qobject_cast<ScriptGrid*>(o)->info()));
                return;
            }
        } else {
            ScriptGlobal::throwError(QString("ScriptGrid::combine: no valid grid object with name: '%1'").arg(it.name()));
            return;
        }
    }
    try {
        // now add names
        Expression expr;
        QVector<double *> vars;
        for (int i=0;i<names.count();++i)
            vars.push_back( expr.addVar(names[i]) );
        expr.setExpression(expression);
        expr.parse();

        // now apply function on grid
        for (int i=0;i<mGrid->count();++i) {
            // set variable values in the expression object
            for (int v=0;v<names.count();++v)
                *vars[v] = grids[v]->valueAtIndex(i);
            double result = expr.execute();
            mGrid->valueAtIndex(i) = result; // write back value
        }
    } catch(const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::combine: expression error: %1").arg(e.message()));
    } catch(const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::combine: expression error: %1").arg(e.what()));
    }
}

QJSValue ScriptGrid::resample(QJSValue grid_object)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::resample: grid not valid or empty!");
        return QJSValue();
    }
    // crop the grid to the extent given by the grid 'grid_object'
    QObject *o = grid_object.toQObject();
    if (o && qobject_cast<ScriptGrid*>(o)) {
        Grid<double> *src = qobject_cast<ScriptGrid*>(o)->grid();
        if (!src || src->isEmpty()) {
            ScriptGlobal::throwError("ScriptGrid::resample: target grid is empty or invalid!");
            return QJSValue();
        }
        try {
            Grid<double> *new_grid = new Grid<double>(src->metricRect(), src->cellsize());
            // now copy content for all cells of the new grid:
            QPointF p;
            for (int i=0;i<new_grid->count();++i) {
                p=new_grid->cellCenterPoint(i);
                if (mGrid->coordValid(p))
                    (*new_grid)[i] = (*mGrid)[p];
                else
                    (*new_grid)[i] = 0.; // should be NA
            }
            // free the original grid...
            if (mOwner)
                delete mGrid;
            mGrid = new_grid;
            mOwner = true;
        } catch (const IException &e) {
            ScriptGlobal::throwError(QString("ScriptGrid::resample: error during resampling: %1").arg(e.message()));
            return QJSValue();
        } catch (const std::exception &e) {
            ScriptGlobal::throwError(QString("ScriptGrid::resample: error during resampling: %1").arg(e.what()));
            return QJSValue();
        }
    } else {
        ScriptGlobal::throwError("ScriptGrid::resample: grid_object is not a valid grid!");
        return QJSValue();
    }

    QJSValue jsgrid = GlobalSettings::instance()->scriptEngine()->newQObject(this);
    return jsgrid;
}

void ScriptGrid::aggregate(int factor)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::aggregate: grid not valid or empty!");
        return;
    }
    if (factor <= 0) {
        ScriptGlobal::throwError(QString("ScriptGrid::aggregate: factor must be > 0 (got %1).").arg(factor));
        return;
    }
    try {
        Grid<double> res = mGrid->averaged(factor);
        // make a copy on the heap
        Grid<double> *new_grid = new Grid<double>(res);
        // delete the old data, and use the new data instead
        if (mOwner)
            delete mGrid;
        mGrid = new_grid;
        mOwner = true;
    } catch (const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::aggregate: %1").arg(e.message()));
    } catch (const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::aggregate: %1").arg(e.what()));
    }
}

QJSValue ScriptGrid::values()
{
    if (!mGrid || mGrid->count()<=0) {
        ScriptGlobal::throwError("ScriptGrid::values: not a valid grid!");
        return QJSValue();
    }
    QJSValue array = GlobalSettings::instance()->scriptEngine()->newArray(mGrid->count());
    for (int i=0;i<mGrid->count(); ++i)
        array.setProperty(i, mGrid->valueAtIndex(i));

    return array;
}

double ScriptGrid::sum(QString expression)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::sum: grid not valid or empty!");
        return -1.;
    }

    try {
        Expression expr;
        double *varptr = expr.addVar(mVariableName);
        expr.setExpression(expression);
        expr.parse();

        // now apply function on grid
        double sum = 0.;
        for (double *p = mGrid->begin(); p!=mGrid->end(); ++p) {
            *varptr = *p;
            sum += expr.execute();
        }
        return sum;
    } catch(const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::sum: error with expression '%1': %2").arg(expression).arg(e.message()));
        return -1.;
    } catch(const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::sum: error with expression '%1': %2").arg(expression).arg(e.what()));
        return -1.;
    }
}

void ScriptGrid::sumTrees(QString expression, QString filter)
{
    if (!mGrid || mGrid->isEmpty()) {
        ScriptGlobal::throwError("ScriptGrid::sumTrees: grid not valid or empty!");
        return;
    }

    mGrid->wipe();

    try {
        TreeWrapper tw;
        Expression custom_expr;
        custom_expr.setExpression(expression);
        custom_expr.setModelObject(&tw);

        Expression filterexpr;
        bool do_filter = !filter.isEmpty();
        filterexpr.setExpression(filter);
        filterexpr.setModelObject(&tw);

        AllTreeIterator ati(GlobalSettings::instance()->model());
        while (Tree *t = ati.next()) {
            // only trees on the grid area:
            if (!mGrid->coordValid(t->position()))
                continue;

            // apply filter
            tw.setTree(t);
            if (do_filter && !filterexpr.execute())
                continue;

            // calculate
            double val=custom_expr.execute();
            mGrid->valueAt(t->position()) += val;
        }
    } catch(const IException &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::sumTrees: an error occurred: %1").arg(e.message()));
    } catch(const std::exception &e) {
        ScriptGlobal::throwError(QString("ScriptGrid::sumTrees: an error occurred: %1").arg(e.what()));
    }
}

