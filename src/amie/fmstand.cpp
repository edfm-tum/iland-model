#include "amie_global.h"
#include "fmstand.h"

#include "fmunit.h"
#include "management.h"
#include "fmtreelist.h"
#include "forestmanagementengine.h"
#include "mapgrid.h"
#include "fmstp.h"
#include "scheduler.h"

#include "tree.h"
#include "species.h"

#include "debugtimer.h"

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
    mStems = 0.;
    mScheduledHarvest = 0.;

    mCurrentIndex=-1;
}

void FMStand::initialize(FMSTP *stp)
{
    mSTP = stp;
    // copy activity flags
    mStandFlags = stp->defaultFlags();
    mCurrentIndex=-1;
    mYearsToWait=0;
    mContextStr = QString("S%2Y%1:").arg(ForestManagementEngine::instance()->currentYear()).arg(id()); // initialize...

    // find out the first activity...
    int min_years_to_wait = 100000;
    for (int i=0;i<mStandFlags.count(); ++i) {
        if (!mStandFlags[i].enabled() || !mStandFlags[i].active())
            continue;
        // set active to false which have already passed
        if (!mStandFlags[i].activity()->schedule().absolute && mStandFlags[i].activity()->latestSchedule(stp->rotationLength()) < age()) {
            mStandFlags[i].setActive(false);
        } else {
            int delta = mStandFlags[i].activity()->earlistSchedule(stp->rotationLength()) - age();
            if (mStandFlags[i].activity()->schedule().absolute)
                delta += age(); // absolute timing: starting from 0

            if (delta<min_years_to_wait) {
                min_years_to_wait = qMax(delta,0); // limit to 0 years
                mCurrentIndex = i; // first activity to execute
            }
        }
    }
    if (min_years_to_wait<100000)
        sleep(min_years_to_wait);

    // call onInit handler on the level of the STP
    stp->events().run(QStringLiteral("onInit"), this);
    if (mCurrentIndex>-1)
        mStandFlags[mCurrentIndex].activity()->events().run(QStringLiteral("onEnter"), this);

}

bool relBasalAreaIsHigher(const SSpeciesStand &a, const SSpeciesStand &b)
{
    return a.relBasalArea > b.relBasalArea;
}

void FMStand::reload()
{
    DebugTimer t("FMStand::reload");
    // load all trees that are located on this stand
    mTotalBasalArea = 0.;
    mVolume = 0.;
    mAge = 0.;
    mStems = 0.;
    FMTreeList trees;
    trees.setStand(this);
    mSpeciesData.clear();

    // load all trees of the forest stand
    trees.loadAll();

    //qDebug() << "fmstand-reload: load trees from map:" << t.elapsed();
    // use: value_per_ha = value_stand * area_factor
    double area_factor = 10000. / ForestManagementEngine::standGrid()->area(mId);
    const QVector<QPair<Tree*, double> > &treelist = trees.trees();
    for ( QVector<QPair<Tree*, double> >::const_iterator it=treelist.constBegin(); it!=treelist.constEnd(); ++it) {
        double ba = it->first->basalArea() * area_factor;
        mTotalBasalArea+=ba;
        mVolume += it->first->volume() * area_factor;
        mAge += it->first->age()*ba;
        mStems++;
        SSpeciesStand &sd = speciesData(it->first->species());
        sd.basalArea += ba;
    }
    if (mTotalBasalArea>0.) {
        mAge /= mTotalBasalArea;
        for (int i=0;i<mSpeciesData.count();++i) {
            mSpeciesData[i].relBasalArea =  mSpeciesData[i].basalArea / mTotalBasalArea;
        }
    }
    mStems *= area_factor; // convert to stems/ha
    // sort species data by relative share....
    std::sort(mSpeciesData.begin(), mSpeciesData.end(), relBasalAreaIsHigher);
}

double FMStand::area() const
{
    return ForestManagementEngine::standGrid()->area(mId)/10000.;
}


bool FMStand::execute()
{
    //  the age of the stand increases by one
    mAge++;

    // do nothing if we are still waiting (sleep)
    if (mYearsToWait>0) {
        if (--mYearsToWait > 0) {
            return false;
        }
    }
    if (trace()) mContextStr = QString("S%2Y%1:").arg(ForestManagementEngine::instance()->currentYear()).arg(id());

    // what to do if there is no active activity??
    if (mCurrentIndex==-1) {
        if (trace()) qCDebug(abe) << context() << "*** No action - no currently active activity ***";
        return false;
    }
    if (trace()) qCDebug(abe) << context() << "*** begin execution, name:" << currentActivity()->name();

    // do nothing if for the stand an activity is currently active in the scheduler
    if (currentFlags().isPending()) {
        if (trace()) qCDebug(abe) << context() << "*** No action - stand in the scheduler. ***";
        return false;
    }

    // do nothing if the the current year is not within the activities window of opportunity
    double p_schedule = currentActivity()->scheduleProbability(this);
    if (p_schedule == 0.) {
        if (trace()) qCDebug(abe)<< context()  << "*** No action - Schedule probability 0. ***";
        return false;
    }

    // check if there are some constraints that prevent execution....
    reload(); // we need to renew the stand data
    double p_execute = currentActivity()->execeuteProbability(this);
    if (p_execute == 0.) {
        if (trace()) qCDebug(abe)<< context() << "*** No action - Constraints preventing execution. ***";
        return false;
    }

    // ok, we should execute the current activity.
    // if it is not scheduled, it is executed immediately, otherwise a ticket is created.
    if (currentFlags().isScheduled()) {
        // ok, we schedule the current activity
        if (trace()) qCDebug(abe)<< context() << "adding ticket for execution.";
        currentFlags().setIsPending(true);
        mScheduledHarvest = 0.;
        bool should_schedule = currentActivity()->evaluate(this);
        if (trace())
            qCDebug(abe) << context() << "evaluated stand. add a ticket:" << should_schedule;
        if (should_schedule) {
            mUnit->scheduler()->addTicket(this, &currentFlags(), p_schedule, p_execute );
        }
        return should_schedule;
    } else {
        // execute immediately
        if (trace()) qCDebug(abe) << context() << "executing activty" << currentActivity()->name();
        mScheduledHarvest = 0.;
        bool executed = currentActivity()->execute(this);
        currentFlags().setIsPending(false);
        currentFlags().setActive(false); // done; TODO: check for repeating activities
        afterExecution(!executed); // check what comes next for the stand
        return executed;
    }
}


bool FMStand::afterExecution(bool cancel)
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
    if (!cancel)
        currentActivity()->events().run(QStringLiteral("onExecute"),this);
    else
        currentActivity()->events().run(QStringLiteral("onCancel"),this);

    if (indexmin != mCurrentIndex) {
        // call events:
        currentActivity()->events().run(QStringLiteral("onExit"), this);
        if (indexmin>-1 && indexmin<-mStandFlags.count())
            mStandFlags[indexmin].activity()->events().run(QStringLiteral("onEnter"), this);

    }
    mCurrentIndex = indexmin;
    if (mCurrentIndex>-1) {
        int to_sleep = tmin - age();
        if (to_sleep>0)
            sleep(to_sleep);
    }
    mScheduledHarvest = 0.; // reset

    return mCurrentIndex > -1;
}


void FMStand::sleep(int years_to_sleep)
{
    mYearsToWait = qMax(mYearsToWait, qMax(years_to_sleep,0));
}

double FMStand::basalArea(const QString &species_id) const
{
    foreach (const SSpeciesStand &sd, mSpeciesData)
        if (sd.species->id()==species_id)
            return sd.basalArea;
    return 0.;
}

// storage for properties (static)
QHash<const FMStand*, QHash<QString, QJSValue> > FMStand::mStandPropertyStorage;


void FMStand::setProperty(const QString &name, QJSValue value)
{
    // save a property value for the current stand
    mStandPropertyStorage[this][name] = value;
}

QJSValue FMStand::property(const QString &name) const
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
