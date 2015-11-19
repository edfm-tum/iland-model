#ifndef SCRIPTGRID_H
#define SCRIPTGRID_H


#include <QObject>
#include <QJSValue>

#include "grid.h"

class ScriptGrid : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(int width READ width)
    Q_PROPERTY(int height READ height)
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(int cellsize READ cellsize)
    Q_PROPERTY(bool isValid READ isValid)
public:
    explicit ScriptGrid(QObject *parent = 0);
    explicit ScriptGrid(Grid<double> *grid) { mVariableName="x"; mGrid = grid; mCreated++; }
    void setGrid(Grid<double> *grid) { mGrid = grid; }
    ~ScriptGrid();
    static QJSValue createGrid(Grid<double> *grid, QString name=QString());

    QString name() const {return mVariableName;}
    Grid<double> *grid() { return mGrid; }

    int width() const { return mGrid?mGrid->sizeX():-1; }
    int height() const { return mGrid?mGrid->sizeY():-1; }
    int count() const { return mGrid?mGrid->count():-1; }
    int cellsize() const { return mGrid?mGrid->cellsize():-1; }
    bool isValid() const { return mGrid?!mGrid->isEmpty():false; }

signals:

public slots:
    void setName(QString arg) { mVariableName = arg; }
    /// create a copy of the current grid and return a new script object
    QJSValue copy();
    /// fill the grid with 0-values
    void clear();

    /// draw the map
    void paint(double min_val, double max_val);

    /// save to a file as ESRI ASC raster grid (relativ to project file)
    void save(QString fileName);

    /// apply a function on the values of the grid, thus modifiying the grid (see the copy() function).
    /// The function is given as an Expression and is run for each cell of the grid.
    void apply(QString expression);

    /// combine multiple grids, and calculate the result of 'expression'
    void combine(QString expression, QJSValue grid_object);


    /// apply the expression "expression" on all pixels of the grid and return the sum of the values
    double sum(QString expression);

    /// access values of the grid
    double value(int x, int y) const {return (isValid() && mGrid->isIndexValid(x,y)) ? mGrid->valueAtIndex(x,y) : -1.;}
    /// write values to the grid
    void setValue(int x, int y, double value) const { if(isValid() && mGrid->isIndexValid(x,y)) mGrid->valueAtIndex(x,y)=value;}


private:
    Grid<double> *mGrid;
    QString mVariableName;
    static int mCreated;
    static int mDeleted;
};

#endif // SCRIPTGRID_H
