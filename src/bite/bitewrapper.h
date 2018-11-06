#ifndef BITEWRAPPER_H
#define BITEWRAPPER_H

#include "expressionwrapper.h"
#include "grid.h"
#include <QStringList>
#include <QVector>

namespace BITE {
class BiteCell;

class BiteWrapper: public ExpressionWrapper
{
public:
    BiteWrapper(): mCell(nullptr)  { buildVarList(); }
    BiteWrapper(const BiteCell *cell):  mCell(cell) { buildVarList(); }
    void setCell(BiteCell *cell) {mCell = cell; }
    const BiteCell *cell() const { return mCell; }

    void registerVariable();
    void registerGridVar(Grid<double> *grid, QString var_name);

    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    enum EVarType { VarDoubleGrid, VarNone };
    void buildVarList();
    double valueActivity(const int variableIndex);
    double valueStand(const int variableIndex);
    double valueSite(const int variableIndex);
    const BiteCell *mCell;
    QStringList mVariables;
    QVector< QPair<EVarType, void*> > mVarObj;
};



} // end namespace

#endif // BITEWRAPPER_H
