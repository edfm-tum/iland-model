#include "bitewrapper.h"

#include "bitecell.h"

#include <QStringList>

namespace BITE {


BiteWrapperCore::~BiteWrapperCore()
{
    //for (auto &s : mVarObj) {
    //    if (s.first == VarDoubleGrid)
    //        delete static_cast<Grid<double>* >(s.second);
    //}
    mVarObj.clear();
}

void BiteWrapperCore::registerGridVar(Grid<double> *grid, QString var_name)
{
    if (mVariables.contains(var_name))
        throw IException(QString("Variable '%1' (for a grid) already in the list of BiteCell variables!").arg(var_name));

    mVariables.push_back(var_name);
    mVarObj.push_back( QPair<EVarType, void*> (VarDoubleGrid, static_cast<void*>(grid)) );
}

const QStringList BiteWrapperCore::getVariablesList()
{
    return mVariables;
}

double BiteWrapperCore::valueCell(const int variableIndex, const BiteCell *cell)
{
    switch (mVarObj[variableIndex].first) {
    case VarDoubleGrid: {
        Grid<double>*g = static_cast<Grid<double>* >( mVarObj[variableIndex].second );
        return g->constValueAtIndex(cell->index());
    }

    case VarNone:
        switch (variableIndex) {
        case 0: return cell->index();
        case 1: return cell->isActive() ? 1. : 0.;
        case 2: return cell->isSpreading() ? 1. : 0.;
        case 3: return cell->yearsLiving();
        default: throw IException("Invalid variable index");
        }

    }
    return 0.;
}

void BiteWrapperCore::setValueCell(const int variableIndex, const BiteCell *cell, double new_value)
{
    if (variableIndex<0 || variableIndex>=mVariables.size())
        throw IException("Invald setValueCell index");

    switch (mVarObj[variableIndex].first) {
    case VarDoubleGrid: {
        Grid<double>*g = static_cast<Grid<double>* >( mVarObj[variableIndex].second );
        g->valueAtIndex(cell->index()) = new_value;
        return;
    }

    case VarNone: {
        BiteCell *c = const_cast<BiteCell*>(cell);
        switch (variableIndex) {
        case 0: throw IException("setValueCell: read-only property: index");
        case 1: c->setActive( new_value == 1. ); break;
        case 2: c->setSpreading( new_value == 1. ); break;
        case 3: throw IException("setValueCell: read-only property: yearsLiving");
        }
        return;
    }
    }

    qDebug() << "setValueCell called";
}

void BiteWrapperCore::buildVarList()
{
    // standard variables
    mVariables.push_back("index"); // index 0
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));
    mVariables.push_back("active"); // index 1
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));
    mVariables.push_back("spreading"); // index 2
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));
    mVariables.push_back("yearsLiving"); // index 3
    mVarObj.push_back(QPair<EVarType, void*>(VarNone, nullptr));

}




} // end namespace
