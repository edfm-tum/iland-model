#include "global.h"
#include "expressionwrapper.h"

#include "tree.h"
#include "ressourceunit.h"
#include <QtCore>

ExpressionWrapper::ExpressionWrapper()
{
}
const QStringList ExpressionWrapper::getVariablesList()
{
    throw IException("expression wrapper reached base getVariableList");
}
double ExpressionWrapper::value(const int variableIndex)
{
    throw IException("expression wrapper reached base getValue");
}

QStringList treeVarList=QStringList() << "id" << "dbh" << "height" << "ruindex" << "x" << "y";
const QStringList TreeWrapper::getVariablesList()
{
    return treeVarList;
}

const int TreeWrapper::variableIndex(const QString &variableName)
{
    return getVariablesList().indexOf(variableName);
}

double TreeWrapper::value(const int variableIndex)
{
    Q_ASSERT(mTree!=0);
    switch (variableIndex) {
        // id
        case 0: return double(mTree->id());
        // dbh
        case 1: return mTree->dbh();
            // height
        case 2: return mTree->height();
            // ressource unit index

        case 3: return (double) mTree->ru()->index();
        case 4: return mTree->position().x();
        case 5: return mTree->position().y();
    }
    throw IException("TreeWrapper::getValue: invalid index");
}
