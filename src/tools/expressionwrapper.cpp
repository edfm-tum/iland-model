/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

/** @class ExpressionWrapper
  @ingroup tools
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
#include "soil.h"
#include "climate.h"

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
                        << "afoliagemass" << "species" // 18, 19
                        << "basalarea" << "crownarea" // 20, 21
                        << "markharvest" << "markcut" << "markcrop" << "markcompetitor"; // 22-25

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
    case 20: return mTree->basalArea();
    case 21: return mTree->crownRadius()*mTree->crownRadius()*M_PI; // area (m2) of the crown
    case 22: return mTree->isMarkedForHarvest()?1:0; // markharvest
    case 23: return mTree->isMarkedForCut()?1:0; // markcut
    case 24: return mTree->isMarkedAsCropTree()?1:0; // markcrop
    case 25: return mTree->isMarkedAsCropCompetitor()?1:0; // markcompetitor
    }
    return ExpressionWrapper::value(variableIndex);
}


////////////////////////////////////////////////
//// ResourceUnit Wrapper
////////////////////////////////////////////////

QStringList ruVarList=QStringList() << baseVarList << "id" << "totalEffectiveArea"
                      << "nitrogenAvailable" << "soilDepth" << "stockedArea" << "stockableArea"
                      << "count" << "volume" << "avgDbh" << "avgHeight" << "basalArea"
                      << "leafAreaIndex" << "aging" << "cohortCount" << "saplingCount" << "saplingAge"
                      << "canopyConductance"
                      << "soilC" << "soilN"
                      << "snagC" << "index" << "meanTemp" << "annualPrecip" << "annualRad";

const QStringList RUWrapper::getVariablesList()
{
    return ruVarList;
}


double RUWrapper::value(const int variableIndex)
{
    Q_ASSERT(mRU!=0);

    switch (variableIndex - baseVarListCount) {
    case 0: return mRU->id(); // id from grid
    case 1: return mRU->mEffectiveArea_perWLA;
    case 2: return mRU->mUnitVariables.nitrogenAvailable;
    case 3: return mRU->waterCycle()->soilDepth();
    case 4: return mRU->stockedArea();
    case 5: return mRU->stockableArea();
    case 6: return mRU->mStatistics.count();
    case 7: return mRU->mStatistics.volume();
    case 8: return mRU->mStatistics.dbh_avg();
    case 9: return mRU->mStatistics.height_avg();
    case 10: return mRU->mStatistics.basalArea();
    case 11: return mRU->mStatistics.leafAreaIndex();
    case 12: return mRU->mAverageAging;
    case 13: return mRU->statistics().cohortCount();
    case 14: return mRU->statistics().saplingCount();
    case 15: return mRU->statistics().saplingAge();
    case 16: return mRU->waterCycle()->canopyConductance();
        // soil C + soil N
    case 17: if (mRU->soil()) return mRU->soil()->youngLabile().C + mRU->soil()->youngRefractory().C + mRU->soil()->oldOrganicMatter().C; else return 0.;
    case 18: if (mRU->soil()) return mRU->soil()->youngLabile().N + mRU->soil()->youngRefractory().N + mRU->soil()->oldOrganicMatter().N; else return 0.;
        // snags
    case 19: if (mRU->snag()) return mRU->snag()->totalCarbon(); else return 0.;
    case 20: return mRU->index(); // numeric index
    case 21: return mRU->climate()->meanAnnualTemperature(); // mean temperature
    case 22: { double psum=0;
        for (int i=0;i<12;++i)
            psum+=mRU->climate()->precipitationMonth()[i];
        return psum; }
    case 23: return mRU->climate()->totalRadiation();

    }
    return ExpressionWrapper::value(variableIndex);
}
