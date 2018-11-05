#ifndef BITEWRAPPER_H
#define BITEWRAPPER_H

#include "expressionwrapper.h"


namespace BITE {
class BiteCell;

class BiteWrapper: public ExpressionWrapper
{
public:
    BiteWrapper(): mCell(nullptr)  {}
    BiteWrapper(const BiteCell *cell):  mCell(cell) {}
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    void buildVarList();
    double valueActivity(const int variableIndex);
    double valueStand(const int variableIndex);
    double valueSite(const int variableIndex);
    const BiteCell *mCell;
};



} // end namespace

#endif // BITEWRAPPER_H
