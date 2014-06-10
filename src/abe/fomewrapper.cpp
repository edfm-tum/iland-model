#include "global.h"
#include "fomewrapper.h"

#include "activity.h"
#include "fmstand.h"
#include "fmunit.h"
namespace ABE {

// definition of variables
// (1) variables of activites

// (2) stand variables
QStringList standVarList=QStringList() << "basalArea" << "age"  << "absoluteAge" << "nspecies" << "volume";

// (3) site variables
QStringList siteVarList=QStringList() << "annualIncrement" << "harvestMode" << "U";
int siteVarListOffset = standVarList.count(); // stand vars start here...


// finally: combine all varibles:
QStringList allVarList;

// setup the variable names
// we use "__" internally (instead of .)
void FOMEWrapper::buildVarList()
{
    allVarList.clear();

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


    return valueStand(variableIndex);
}



double FOMEWrapper::valueStand(const int variableIndex)
{
    //<< "basalArea" << "age"  << "absoluteAge" << "speciesCount" << "volume"
    switch (variableIndex) {
    case 0: return mStand->basalArea(); // "basalArea"
    case 1: return mStand->age(); // mean age, "age"
    case 2: return mStand->absoluteAge(); // years since begin of rotation, "absoluteAge"
    case 3: return mStand->nspecies(); // species richness, "nspecies"
    case 4: return mStand->volume(); // total standing volume, m3/ha, "volume"
    default: return 0;
    }
}

double FOMEWrapper::valueSite(const int variableIndex)
{
    switch (variableIndex) {
    case 2: return 120; // just testing
    default: return 0;
    }
}

} // namespace