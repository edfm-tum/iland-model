/** @class ExpressionWrapper is the base class of objects that can be used with Expressions.
  Derived from ExpressionWrapper are wrappers for e.g. Trees or ResourceUnits.
  They must provide a getVariablesList() and a value() function.
  Note: the must also provide "virtual double value(const QString &variableName) { return value(variableName); }"
      because it seems not possible C++ wise to use functions from derived and base class simultaneously that only differ in the
      argument signature.

  */
#include "global.h"
#include "expressionwrapper.h"

#include "tree.h"
#include "resourceunit.h"
#include "species.h"
#include <QtCore>

ExpressionWrapper::ExpressionWrapper()
{
}
// must be overloaded!
const QStringList ExpressionWrapper::getVariablesList()
{
    throw IException("expression wrapper reached base getVariableList");
}
// must be overloaded!
double ExpressionWrapper::value(const int variableIndex)
{
    throw IException(QString("expression wrapper reached base getValue index %1").arg(variableIndex));
}

int ExpressionWrapper::variableIndex(const QString &variableName)
{
    return getVariablesList().indexOf(variableName);
}

double ExpressionWrapper::valueByName(const QString &variableName)
{
    int idx = variableIndex(variableName);
    return value(idx);
}




QStringList treeVarList=QStringList() << "id" << "dbh" << "height" << "ruindex" // 0..3
                        << "x" << "y" << "volume" << "lri" << "la" << "leafarea" << "lightresponse" // 4-10
                        << "woodymass" << "rootmass" << "foliagemass" << "age" << "opacity" // 11-15
                        << "dead" << "stress" << "deltad" //16-18
                        << "afoliagemass"; // 19
const QStringList TreeWrapper::getVariablesList()
{
    return treeVarList;
}


double TreeWrapper::value(const int variableIndex)
{
    Q_ASSERT(mTree!=0);

    switch (variableIndex) {
    case 0: return double(mTree->id()); // id
    case 1: return mTree->dbh(); // dbh
    case 2: return mTree->height(); // height
    case 3: return (double) mTree->ru()->index(); // ruindex
    case 4: return mTree->position().x(); // x
    case 5: return mTree->position().y(); // y
    case 6: return mTree->volume(); // volume
    case 7: return mTree->lightResourceIndex(); // lri
    case 8: case 9: return mTree->mLeafArea;
    case 10: return mTree->mLightResponse;
    case 11: return mTree->mWoodyMass;
    case 12: return mTree->mRootMass;
    case 13: return mTree->mFoliageMass;
    case 14: return mTree->age();
    case 15: return mTree->mOpacity;
    case 16: return mTree->isDead()?1.:0.;
    case 17: return mTree->mStressIndex;
    case 18: return mTree->mDbhDelta; // increment of last year
    case 19: return mTree->species()->biomassFoliage(mTree->dbh()); // allometric foliage
    }
    throw IException("TreeWrapper::getValue: invalid index");
}


////////////////////////////////////////////////
//// ResourceUnit Wrapper
////////////////////////////////////////////////

QStringList ruVarList=QStringList() << "id" << "la" << "total_radiation";

const QStringList RUWrapper::getVariablesList()
{
    return ruVarList;
}


double RUWrapper::value(const int variableIndex)
{
    Q_ASSERT(mRU!=0);

    switch (variableIndex) {
    case 0: return mRU->index();
    case 1: return mRU->mAggregatedLA;
    case 2: return mRU->mRadiation_m2;
    }
    throw IException("RUWrapper::getValue: invalid index");
}
