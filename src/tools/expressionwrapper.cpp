#include "global.h"
#include "expressionwrapper.h"

#include "tree.h"
#include "ressourceunit.h"
#include "species.h"
#include <QtCore>

ExpressionWrapper::ExpressionWrapper()
{
}
int ExpressionWrapper::variableIndex(const QString &variableName)
{
    throw IException(QString("expression wrapper reached base variableIndex name %1").arg(variableName));
}

const QStringList ExpressionWrapper::getVariablesList()
{
    throw IException("expression wrapper reached base getVariableList");
}

double ExpressionWrapper::value(const int variableIndex)
{
    throw IException(QString("expression wrapper reached base getValue index %1").arg(variableIndex));
}
double ExpressionWrapper::value(const QString &variableName)
{
    throw IException(QString("expression wrapper reached base getValue index %1").arg(variableName));
}




QStringList treeVarList=QStringList() << "id" << "dbh" << "height" << "ruindex" // 0..3
                        << "x" << "y" << "volume" << "lri" << "la" << "leafarea" // 4-9
                        << "woodymass" << "rootmass" << "foliagemass" << "opacity" // 10-13
                        << "dead" << "stress" << "deltad" //14-16
                        << "afoliagemass";
const QStringList TreeWrapper::getVariablesList()
{
    return treeVarList;
}

int TreeWrapper::variableIndex(const QString &variableName)
{
    return getVariablesList().indexOf(variableName);
}

double TreeWrapper::value(const QString &variableName)
{
    int idx = variableIndex(variableName);
    return value(idx);
}

double TreeWrapper::value(const int variableIndex)
{
    Q_ASSERT(mTree!=0);
    // testcode....
//    const_cast<Tree*>(mTree)->setFlag(Tree::TreeDead, true);
//    qDebug() << "flagtest" << mTree->dead();
//    const_cast<Tree*>(mTree)->setFlag(Tree::TreeDead, false);
//    qDebug() << mTree->dead();

    switch (variableIndex) {
    case 0: return double(mTree->id()); // id
    case 1: return mTree->dbh(); // dbh
    case 2: return mTree->height(); // height
    case 3: return (double) mTree->ru()->index(); // ruindex
    case 4: return mTree->position().x(); // x
    case 5: return mTree->position().y(); // y
    case 6: return mTree->volume(); // volume
    case 7: return mTree->lightRessourceIndex(); // lri
    case 8: case 9: return mTree->mLeafArea;
    case 10: return mTree->mWoodyMass;
    case 11: return mTree->mRootMass;
    case 12: return mTree->mFoliageMass;
    case 13: return mTree->mOpacity;
    case 14: return mTree->isDead()?1.:0.;
    case 15: return mTree->mStressIndex;
    case 16: return mTree->mDbhDelta; // increment of last year
    case 17: return mTree->species()->biomassFoliage(mTree->dbh()); // allometric foliage
    }
    throw IException("TreeWrapper::getValue: invalid index");
}
