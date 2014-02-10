#include "amie_global.h"
#include "fmstand.h"

#include "fmunit.h"
#include "management.h"
#include "treelist.h"
#include "forestmanagementengine.h"
#include "mapgrid.h"
#include "fmstp.h"

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

void FMStand::initialize(FMSTP *stp)
{
    mSTP = stp;
    // copy activity flags
    mStandFlags = stp->defaultFlags();
    mCurrentIndex=-1;
    mYearsToWait=0;

    // find out the first activity...
    int min_years_to_wait = 100000;
    for (int i=0;i<mStandFlags.count(); ++i) {
        // set active to false which have already passed
        if (mStandFlags[i].activity()->latestSchedule(stp->rotationLength()) < age()) {
            mStandFlags[i].setActive(false);
        } else {
            int delta = mStandFlags[i].activity()->earlistSchedule(stp->rotationLength()) - age();
            if (delta>0 && delta<min_years_to_wait)
                min_years_to_wait = delta;
        }
    }
    if (min_years_to_wait<100000)
        sleep(min_years_to_wait);

    // call onInit handler on the level of the STP
    stp->events().run("onInit", this);

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


bool FMStand::execute()
{
    // do nothing if we are still waiting (sleep)
    if (mYearsToWait>0) {
        if (--mYearsToWait > 0)
            return false;
    }
    // what to do if there is no active activity??
    if (mCurrentIndex==-1)
        return false;

    // do nothing if for the stand an activity is currently active in the scheduler
    if (currentFlags().isPending())
        return false;

    // do nothing if the the current year is not within the activities window of opportunity
    double p_schedule = currentActivity()->scheduleProbability(this);
    if (p_schedule == 0.)
        return false;

    // check if there are some constraints that prevent execution....
    reload(); // we need to renew the stand data
    if (!currentActivity()->canExeceute(this))
        return false;

    // ok, we schedule the current activity
    currentFlags().setIsPending(true);
    ForestManagementEngine::instance()->scheduler().addTicket(this, &currentFlags());
    return true;
}


bool FMStand::afterExecution()
{
    // is called after an activity has run
    int tmin = 10000000;
    int indexmin = -1;
    for (int i=0;i<mStandFlags.count(); ++i) {
        if (mStandFlags[i].isForcedNext()) {
            mStandFlags[i].setForceNext(false); // reset flag
            indexmin = i;
            break; // we "jump" to this activity
        }
        if ( mStandFlags[i].enabled() && mStandFlags[i].active())
            if (mStandFlags[i].activity()->earlistSchedule() < tmin) {
               tmin =  mStandFlags[i].activity()->earlistSchedule();
               indexmin = i;
            }
    }
    mCurrentIndex = indexmin;
    if (mCurrentIndex>-1) {
        int to_sleep = tmin - age();
        if (to_sleep>0)
            sleep(to_sleep);
    }
    return mCurrentIndex > -1;
}

void FMStand::sleep(int years_to_sleep)
{
    mYearsToWait = qMax(mYearsToWait, years_to_sleep);
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
