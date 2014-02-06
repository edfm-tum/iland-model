#include "amie_global.h"
#include "fmstand.h"

#include "fmunit.h"
#include "management.h"
#include "treelist.h"
#include "forestmanagementengine.h"
#include "mapgrid.h"

#include "tree.h"
#include "species.h"

namespace AMIE {

FMStand::FMStand(FMUnit *unit, const int id)
{
    mUnit = unit;
    mId = id;
    mPhase = Activity::Invalid;
    // testing:
    mPhase = Activity::Tending;
    mStandType = 1; // just testing...
    mSTP = 0;
    mVolume = 0.;
    mAge = 0.;
    mTotalBasalArea = 0.;

    mCurrentIndex=0;
}
bool relBasalAreaIsHigher(const SSpeciesStand &a, const SSpeciesStand &b)
{
    return a.relBasalArea > b.relBasalArea;
}

void FMStand::reload()
{

    // load all trees that are located on this stand
    mTotalBasalArea = 0.;
    mVolume = 0;
    mAge = 0;
    TreeList trees;
    mSpeciesData.clear();
    trees.loadFromMap(ForestManagementEngine::standGrid(), mId);
    // use: value_per_ha = value_stand * area_factor
    double area_factor = 10000. / ForestManagementEngine::standGrid()->area(mId);
    const QList<QPair<Tree*, double> > &treelist = trees.trees();
    for ( QList<QPair<Tree*, double> >::const_iterator it=treelist.constBegin(); it!=treelist.constEnd(); ++it) {
        double ba = it->first->basalArea() * area_factor;
        mTotalBasalArea+=ba;
        mVolume += it->first->volume() * area_factor;
        mAge += it->first->age()*ba;
        SSpeciesStand &sd = speciesData(it->first->species());
        sd.basalArea += ba;
    }
    if (mTotalBasalArea>0.) {
        mAge /= mTotalBasalArea;
        for (int i=0;i<mSpeciesData.count();++i) {
            mSpeciesData[i].relBasalArea =  mSpeciesData[i].basalArea / mTotalBasalArea;
        }
    }
    // sort species data by relative share....
    std::sort(mSpeciesData.begin(), mSpeciesData.end(), relBasalAreaIsHigher);
}

double FMStand::basalArea(const QString &species_id) const
{
    foreach (const SSpeciesStand &sd, mSpeciesData)
        if (sd.species->id()==species_id)
            return sd.basalArea;
    return 0.;
}

// storage for properties (static)
QHash<FMStand*, QHash<QString, QJSValue> > FMStand::mStandPropertyStorage;


void FMStand::setProperty(const QString &name, QJSValue value)
{
    // save a property value for the current stand
    mStandPropertyStorage[this][name] = value;
}

QJSValue FMStand::property(const QString &name)
{
    // check if values are already stored for the current stand
    if (!mStandPropertyStorage.contains(this))
        return QJSValue();
    // check if something is stored for the property name (return a undefined value if not)
    if (!mStandPropertyStorage[this].contains(name))
        return QJSValue();
    return mStandPropertyStorage[this][name];
}

SSpeciesStand &FMStand::speciesData(const Species *species)
{
    for (int i=0;i<mSpeciesData.count(); ++i)
        if (mSpeciesData[i].species == species)
            return mSpeciesData[i];

    mSpeciesData.append(SSpeciesStand());
    mSpeciesData.last().species = species;
    return mSpeciesData.last();
}


} // namespace
