#include "bitewrapper.h"

#include "bitecell.h"

#include <QStringList>

namespace BITE {


void BiteWrapper::registerGridVar(Grid<double> *grid, QString var_name)
{
    if (mVariables.contains(var_name))
        throw IException(QString("Variable '%1' (for a grid) already in the list of BiteCell variables!").arg(var_name));

    mVariables.push_back(var_name);
    mVarObj.push_back( QPair<EVarType, void*> (VarDoubleGrid, static_cast<void*>(grid)) );
}

const QStringList BiteWrapper::getVariablesList()
{
    return mVariables;
}

double BiteWrapper::value(const int variableIndex)
{
    switch (mVarObj[variableIndex].first) {
    case VarDoubleGrid: {
        Grid<double>*g = static_cast<Grid<double>* >( mVarObj[variableIndex].second );
        return g->constValueAtIndex(mCell->index());
    }

    case VarNone:
        switch (variableIndex) {
        case 0: return mCell->index();
        case 1: return mCell->isActive() ? 1. : 0.;
        default: throw IException("Invalid variable index");
        }

    }
    return 0.;
}

void BiteWrapper::buildVarList()
{
    // standard variables
    mVariables.push_back("index"); // index 0
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));
    mVariables.push_back("active"); // index 1
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));

}



} // end namespace
