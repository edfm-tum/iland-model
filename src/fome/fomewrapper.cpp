#include "global.h"
#include "fomewrapper.h"

#include "activity.h"

// definition of variables
// (1) variables of activites
QStringList activityVarList=QStringList() << "economy" << "experimentation" << "knowledge";

// (2) stand variables
QStringList standVarList=QStringList() << "basalArea" << "age" << "speciesCount" << "volume";
int standVarListOffset = activityVarList.count(); // stand vars start here...

// (3) site variables
QStringList siteVarList=QStringList() << "annualIncrement" << "harvestMode" << "U";
int siteVarListOffset = activityVarList.count() + standVarList.count(); // stand vars start here...


// finally: combine all varibles:
QStringList allVarList;

// setup the variable names
// we use "__" internally (instead of .)
void FOMEWrapper::buildVarList()
{
    allVarList.clear();
    foreach(QString var, activityVarList)
        allVarList.append(QString("activity__%1").arg(var));

    foreach(QString var, standVarList)
        allVarList.append(QString("stand__%1").arg(var));

    foreach(QString var, siteVarList)
        allVarList.append(QString("site__%1").arg(var));
}


const QStringList FOMEWrapper::getVariablesList()
{
    if (allVarList.isEmpty())
        buildVarList();
    return allVarList;
}

double FOMEWrapper::value(const int variableIndex)
{
    // dispatch
    if (variableIndex > siteVarListOffset)
        return valueSite(variableIndex - siteVarListOffset);

    if (variableIndex > standVarListOffset)
        return valueStand(variableIndex - standVarListOffset);

    return valueActivity(variableIndex);
}


double FOMEWrapper::valueActivity(const int variableIndex)
{
    if (!mActivity) return 0;
    switch (variableIndex) {
    case 0: return mActivity->economy();
    case 1: return mActivity->experimentation();
    case 2: return mActivity->knowledge();
    default: return 0;
    }
}

double FOMEWrapper::valueStand(const int variableIndex)
{
    switch (variableIndex) {
    default: return 0;
    }
}

double FOMEWrapper::valueSite(const int variableIndex)
{
    switch (variableIndex) {
    default: return 0;
    }
}
