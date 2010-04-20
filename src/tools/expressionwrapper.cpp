/** @class ExpressionWrapper
  The base class for objects that can be used within Expressions.
  Derived from ExpressionWrapper are wrappers for e.g. Trees or ResourceUnits.
  They must provide a getVariablesList() and a value() function.
  Note: the must also provide "virtual double value(const QString &variableName) { return value(variableName); }"
      because it seems to be not possible in C++ to use functions from derived and base class simultaneously that only differ in the
      argument signature.
  @sa Expression

  */
#include "global.h"
#include "expressionwrapper.h"

#include "tree.h"
#include "resourceunit.h"
#include "species.h"
#include "watercycle.h"
#include "standstatistics.h"

#include <QtCore>

ExpressionWrapper::ExpressionWrapper()
{
}
// must be overloaded!
QStringList baseVarList=QStringList() << "year";
const int baseVarListCount = baseVarList.count();

const QStringList ExpressionWrapper::getVariablesList()
{
    throw IException("expression wrapper reached base getVariableList");
}
// must be overloaded!
double ExpressionWrapper::value(const int variableIndex)
{
    switch (variableIndex) {
        case 0: // year
            return (double) GlobalSettings::instance()->currentYear();
    }
    throw IException(QString("expression wrapper reached base with invalid index index %1").arg(variableIndex));
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




QStringList treeVarList=QStringList() << baseVarList << "id" << "dbh" << "height" << "ruindex" // 0..3
                        << "x" << "y" << "volume" << "lri" <<  "leafarea" << "lightresponse" // 4-9
                        << "woodymass" << "rootmass" << "foliagemass" << "age" << "opacity" // 10-14
                        << "dead" << "stress" << "deltad" //15-17
                        << "afoliagemass" << "speciesindex"; // 18, 19
const QStringList TreeWrapper::getVariablesList()
{
    return treeVarList;
}


double TreeWrapper::value(const int variableIndex)
{
    Q_ASSERT(mTree!=0);

    switch (variableIndex - baseVarListCount) {
    case 0: return double(mTree->id()); // id
    case 1: return mTree->dbh(); // dbh
    case 2: return mTree->height(); // height
    case 3: return (double) mTree->ru()->index(); // ruindex
    case 4: return mTree->position().x(); // x
    case 5: return mTree->position().y(); // y
    case 6: return mTree->volume(); // volume
    case 7: return mTree->lightResourceIndex(); // lri
    case 8: return mTree->mLeafArea;
    case 9: return mTree->mLightResponse;
    case 10: return mTree->mWoodyMass;
    case 11: return mTree->mCoarseRootMass + mTree->mFineRootMass; // sum of coarse and fine roots
    case 12: return mTree->mFoliageMass;
    case 13: return mTree->age();
    case 14: return mTree->mOpacity;
    case 15: return mTree->isDead()?1.:0.;
    case 16: return mTree->mStressIndex;
    case 17: return mTree->mDbhDelta; // increment of last year
    case 18: return mTree->species()->biomassFoliage(mTree->dbh()); // allometric foliage
    case 19: return mTree->species()->index();
    }
    return ExpressionWrapper::value(variableIndex);
}


////////////////////////////////////////////////
//// ResourceUnit Wrapper
////////////////////////////////////////////////

QStringList ruVarList=QStringList() << baseVarList << "id" << "totalEffectiveArea"
                      << "nitrogenAvailable" << "soilDepth" << "stockedArea"
                      << "count" << "volume" << "avgDbh" << "avgHeight" << "basalArea"
                      << "leafAreaIndex" << "aging";

const QStringList RUWrapper::getVariablesList()
{
    return ruVarList;
}


double RUWrapper::value(const int variableIndex)
{
    Q_ASSERT(mRU!=0);

    switch (variableIndex - baseVarListCount) {
    case 0: return mRU->index();
    case 1: return mRU->mEffectiveArea_perWLA;
    case 2: return mRU->mUnitVariables.nitrogenAvailable;
    case 3: return mRU->waterCycle()->soilDepth();
    case 4: return mRU->stockedArea();
    case 5: return mRU->mStatistics.count();
    case 6: return mRU->mStatistics.volume();
    case 7: return mRU->mStatistics.dbh_avg();
    case 8: return mRU->mStatistics.height_avg();
    case 9: return mRU->mStatistics.basalArea();
    case 10: return mRU->mStatistics.leafAreaIndex();
    case 11: return mRU->mAverageAging;

    }
    return ExpressionWrapper::value(variableIndex);
}
