#include "scriptgrid.h"
#include "globalsettings.h"
#include "helper.h"
#include "expression.h"
#include "modelcontroller.h"

#include <QJSEngine>
#include <QJSValueIterator>

int ScriptGrid::mDeleted = 0;
int ScriptGrid::mCreated = 0;

ScriptGrid::ScriptGrid(QObject *parent) : QObject(parent)
{
    mGrid = 0;
    mVariableName = "x"; // default name
    mCreated++;
}

ScriptGrid::~ScriptGrid()
{
    if (mGrid)
        delete mGrid;
    mDeleted++;
    qDebug() << "ScriptGrid::balance: created:" << mCreated << "deleted:" << mDeleted;
}

QJSValue ScriptGrid::createGrid(Grid<double> *grid, QString name)
{
    ScriptGrid *g = new ScriptGrid(grid);
    if (!name.isEmpty())
        g->setName(name);
    QJSValue jsgrid = GlobalSettings::instance()->scriptEngine()->newQObject(g);
    return jsgrid;
}

QJSValue ScriptGrid::copy()
{
    if (!mGrid)
        return QJSValue();

    ScriptGrid *newgrid = new ScriptGrid();
    // copy the data
    Grid<double> *copy_grid = new Grid<double>(*mGrid);
    newgrid->setGrid(copy_grid);

    QJSValue jsgrid = GlobalSettings::instance()->scriptEngine()->newQObject(newgrid);
    return jsgrid;
}

void ScriptGrid::clear()
{
    if (mGrid && !mGrid->isEmpty())
        mGrid->wipe();
}

void ScriptGrid::paint(double min_val, double max_val)
{
    //if (GlobalSettings::instance()->controller())
    //    GlobalSettings::instance()->controller()->addGrid(mGrid, mVariableName, GridViewRainbow, min_val, max_val);
}

void ScriptGrid::save(QString fileName)
{
    if (!mGrid || mGrid->isEmpty())
        return;
    fileName = GlobalSettings::instance()->path(fileName);
    QString result = gridToESRIRaster(*mGrid);
    Helper::saveToTextFile(fileName, result);
    qDebug() << "saved grid " << name() << " to " << fileName;
}

void ScriptGrid::apply(QString expression)
{
    if (!mGrid || mGrid->isEmpty())
        return;

    Expression expr;
    double *varptr = expr.addVar(mVariableName);
    try {
        expr.setExpression(expression);
        expr.parse();
    } catch(const IException &e) {
        qDebug() << "JS - grid:apply(): ERROR: " << e.message();
        return;
    }

    // now apply function on grid
    for (double *p = mGrid->begin(); p!=mGrid->end(); ++p) {
        *varptr = *p;
        *p = expr.execute();
    }

}

void ScriptGrid::combine(QString expression, QJSValue grid_object)
{
    if (!grid_object.isObject()) {
        qDebug() << "ERROR: ScriptGrid::combine(): no valid grids object" << grid_object.toString();
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
            if (grids.last()->isEmpty() || grids.last()->cellsize() != mGrid->cellsize() || grids.last()->rectangle()!=mGrid->rectangle()) {
                qDebug() << "ERROR: ScriptGrid::combine(): the grid " << it.name() << "is empty or has different dimensions.";
                return;
            }
        } else {
            qDebug() << "ERROR: ScriptGrid::combine(): no valid grid object with name:" << it.name();
            return;
        }
    }
    // now add names
    Expression expr;
    QVector<double *> vars;
    for (int i=0;i<names.count();++i)
        vars.push_back( expr.addVar(names[i]) );
    try {
        expr.setExpression(expression);
        expr.parse();
    } catch(const IException &e) {
        qDebug() << "JS - grid:combine(): expression ERROR: " << e.message();
        return;
    }

    // now apply function on grid
    for (int i=0;i<mGrid->count();++i) {
        // set variable values in the expression object
        for (int v=0;v<names.count();++v)
            *vars[v] = grids[v]->valueAtIndex(i);
        double result = expr.execute();
        mGrid->valueAtIndex(i) = result; // write back value
    }
}

double ScriptGrid::sum(QString expression)
{
    if (!mGrid || mGrid->isEmpty())
        return -1.;

    Expression expr;
    double *varptr = expr.addVar(mVariableName);
    try {
        expr.setExpression(expression);
        expr.parse();
    } catch(const IException &e) {
        qDebug() << "JS - grid:apply(): ERROR: " << e.message();
        return -1.;
    }

    // now apply function on grid
    double sum = 0.;
    for (double *p = mGrid->begin(); p!=mGrid->end(); ++p) {
        *varptr = *p;
        sum += expr.execute();
    }
    return sum;
}
