#ifndef BITEWRAPPER_H
#define BITEWRAPPER_H

#include "expressionwrapper.h"
#include "grid.h"
#include <QStringList>
#include <QVector>

namespace BITE {
class BiteCell;

class BiteWrapperCore: public ExpressionWrapper
{
public:
    BiteWrapperCore()  { buildVarList(); }
    ~BiteWrapperCore();

    void registerVariable();
    void registerGridVar(Grid<double> *grid, QString var_name);
    void registerClimateVar(int var_index, QString var_name);

    virtual const QStringList getVariablesList();
    double valueCell(const int variableIndex, const BiteCell* cell);
    void setValueCell(const int variableIndex, const BiteCell* cell, double new_value);

private:
    enum EVarType { VarDoubleGrid, VarNone, VarClimate };
    void buildVarList();
    double valueActivity(const int variableIndex);
    double valueStand(const int variableIndex);
    double valueSite(const int variableIndex);
    QStringList mVariables;
    QVector< QPair<EVarType, void*> > mVarObj;
};

class BiteWrapper: public ExpressionWrapper
{
public:
    BiteWrapper(BiteWrapperCore *wrap, BiteCell *cell=nullptr): mWrap(wrap), mCell(cell) {}
    void setCell(BiteCell *cell) {mCell = cell; }

    virtual const QStringList getVariablesList() { return mWrap->getVariablesList();}
    virtual double value(const int variableIndex) { return mWrap->valueCell(variableIndex, mCell); }
    void setValue(const int variableIndex, double new_value) { mWrap->setValueCell(variableIndex, mCell, new_value); }
private:
    BiteWrapperCore *mWrap;
    const BiteCell *mCell;


};

} // end namespace

#endif // BITEWRAPPER_H
