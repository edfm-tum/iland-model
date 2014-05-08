#include "amie_global.h"
#include "fmstand.h"

#include "fmunit.h"
#include "management.h"
#include "fmtreelist.h"
#include "forestmanagementengine.h"
#include "mapgrid.h"
#include "fmstp.h"
#include "scheduler.h"
#include "fomescript.h"
#include "agent.h"
#include "agenttype.h"

#include "tree.h"
#include "species.h"

#include "debugtimer.h"

namespace ABE {

FMStand::FMStand(FMUnit *unit, const int id)
{
    mUnit = unit;
    mId = id;
    mPhase = Activity::Invalid;

    // testing:
    mPhase = Activity::Tending;
    mStandType = 1; // just testing...

    newRotatation();
    mSTP = 0;
    mVolume = 0.;
    mAge = 0.;
    mTotalBasalArea = 0.;
    mStems = 0.;
    mScheduledHarvest = 0.;
    mHarvested = 0.;
    mDisturbed = 0.;
    mRotationStartYear = 0;
    mLastUpdate = -1.;
    mLastExecution = -1.;

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

    // load data and aggregate averages
    reload();
    mRotationStartYear = ForestManagementEngine::instance()->currentYear() - age();
    // when a stand is initialized, we assume that 20% of the standing volume
    // have been removed already.
    mRemovedVolumeTotal = volume() * 0.2;
    mMAItotal = volume() * 1.2 / absoluteAge();
    mMAIdecade = mMAItotal;
    mLastMAIVolume = volume();

    // find out the first activity...
    int min_years_to_wait = 100000;
    for (int i=0;i<mStandFlags.count(); ++i) {
        // run the onSetup event
        // specifically set 'i' as the activity to be evaluated.
        FomeScript::setExecutionContext(this);
        FomeScript::bridge()->activityObj()->setActivityIndex(i);
        mStandFlags[i].activity()->events().run(QStringLiteral("onSetup"), 0);

        if (!mStandFlags[i].enabled() || !mStandFlags[i].active())
            continue;
        // set active to false which have already passed
        if (!mStandFlags[i].activity()->isRepeatingActivity()) {
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
    }
    if (mCurrentIndex==-1) {
        // the stand is "outside" the time frames provided by the activities.
        // set the last activity with "force" = true as the active
        for (int i=mStandFlags.count()-1;i>=0; --i)
            if (mStandFlags[i].enabled() && mStandFlags[i].activity()->schedule().force_execution==true) {
                mCurrentIndex = i;
                break;
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

void FMStand::reload(bool force)
{
    if (!force && mLastUpdate == ForestManagementEngine::instance()->currentYear())
        return;

    DebugTimer t("AMIE:FMStand::reload");
    // load all trees that are located on this stand
    mTotalBasalArea = 0.;
    mVolume = 0.;
    mAge = 0.;
    mStems = 0.;
    mLastUpdate = ForestManagementEngine::instance()->currentYear();
    mSpeciesData.clear();

    // load all trees of the forest stand (use the treelist of the current execution context)
    FMTreeList *trees = ForestManagementEngine::instance()->scriptBridge()->treesObj();
    trees->setStand(this);
    trees->loadAll();

    //qDebug() << "fmstand-reload: load trees from map:" << t.elapsed();
    // use: value_per_ha = value_stand * area_factor
    double area_factor = 1. / area();
    const QVector<QPair<Tree*, double> > &treelist = trees->trees();
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

double FMStand::absoluteAge() const
{
    return ForestManagementEngine::instance()->currentYear() - mRotationStartYear;
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
    if (trace())
        mContextStr = QString("S%2Y%1:").arg(ForestManagementEngine::instance()->currentYear()).arg(id());

    // what to do if there is no active activity??
    if (mCurrentIndex==-1) {
        if (trace())
            qCDebug(amie) << context() << "*** No action - no currently active activity ***";
        return false;
    }
    if (trace())
        qCDebug(amie) << context() << "*** start evaulate activity:" << currentActivity()->name();

    // do nothing if there is already an activity in the scheduler
    if (currentFlags().isPending()) {
        if (trace())
            qCDebug(amie) << context() << "*** No action - stand in the scheduler. ***";
        return false;
    }

    // do nothing if the the current year is not within the window of opportunity of the activity
    double p_schedule = currentActivity()->scheduleProbability(this);
    if (p_schedule == -1.) {
        if (trace())
            qCDebug(amie)<< context()  << "*** Activity expired. ***";
    }
    if (p_schedule>=0. && p_schedule < 0.00001) {
        if (trace())
            qCDebug(amie)<< context()  << "*** No action - Schedule probability 0. ***";
        return false;
    }


    // we need to renew the stand data
    reload();


    // check if there are some constraints that prevent execution....
    double p_execute = currentActivity()->execeuteProbability(this);
    if (p_execute == 0.) {
        if (trace())
            qCDebug(amie)<< context() << "*** No action - Constraints preventing execution. ***";
        return false;
    }

    // ok, we should execute the current activity.
    // if it is not scheduled, it is executed immediately, otherwise a ticket is created.
    if (currentFlags().isScheduled()) {
        // ok, we schedule the current activity
        if (trace())
            qCDebug(amie)<< context() << "adding ticket for execution.";

        mScheduledHarvest = 0.;
        bool should_schedule = currentActivity()->evaluate(this);
        if (trace())
            qCDebug(amie) << context() << "evaluated stand. add a ticket:" << should_schedule;
        if (should_schedule) {
            mUnit->scheduler()->addTicket(this, &currentFlags(), p_schedule, p_execute );
        } else {
            // cancel the activity
            currentFlags().setActive(false);
            afterExecution(true);
        }
        return should_schedule;
    } else {
        // execute immediately
        if (trace())
            qCDebug(amie) << context() << "executing activty" << currentActivity()->name();
        mScheduledHarvest = 0.;
        bool executed = currentActivity()->execute(this);
        if (!currentActivity()->isRepeatingActivity()) {
            currentFlags().setActive(false);
            afterExecution(!executed); // check what comes next for the stand
        }
        return executed;
    }
}

bool FMStand::executeActivity(Activity *act)
{
    int old_activity_index = mCurrentIndex;

    int new_index = stp()->activityIndex(act);
    bool result;
    if (new_index>-1) {
        mCurrentIndex = new_index;
        int old_years = mYearsToWait;
        mYearsToWait = 0;
        result = execute();
        mAge--; // undo modification of age
        mYearsToWait = old_years; // undo...
    }
    mCurrentIndex = old_activity_index;
    return result;
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
    }

    if (indexmin == -1) {
        // check if a restart is needed
        // TODO: find a better way!!
        if (currentFlags().isFinalHarvest()) {
            // we have reached the last activity
            for (int i=0;i<mStandFlags.count(); ++i)
                mStandFlags[i].setActive(true);
            newRotatation();
            reload();
        }

        // look for the next (enabled) activity.
        for (int i=0;i<mStandFlags.count(); ++i) {
            if ( mStandFlags[i].enabled() && mStandFlags[i].active())
                if (mStandFlags[i].activity()->earlistSchedule() < tmin) {
                    tmin =  mStandFlags[i].activity()->earlistSchedule();
                    indexmin = i;
                }
        }
    }

    if (!cancel)
        currentActivity()->events().run(QStringLiteral("onExecuted"),this);
    else
        currentActivity()->events().run(QStringLiteral("onCancel"),this);

    if (indexmin != mCurrentIndex) {
        // call events:
        currentActivity()->events().run(QStringLiteral("onExit"), this);
        if (indexmin>-1 && indexmin<mStandFlags.count())
            mStandFlags[indexmin].activity()->events().run(QStringLiteral("onEnter"), this);

    }
    mCurrentIndex = indexmin;
    if (mCurrentIndex>-1) {
        int to_sleep = tmin - absoluteAge();
        if (to_sleep>0)
            sleep(to_sleep);
    }
    mScheduledHarvest = 0.; // reset

    mLastExecution = ForestManagementEngine::instance()->currentYear();

    return mCurrentIndex > -1;
}

void FMStand::addTreeRemoval(Tree *tree, int reason)
{
    double removed_volume = tree->volume();

    // for MAI calculations: store removal regardless of the reason
    mRemovedVolumeDecade+=removed_volume / area();
    mRemovedVolumeTotal+=removed_volume / area();
    mRemovedVolumeTicks++;

    Tree::TreeRemovalType r = Tree::TreeRemovalType (reason);
    if (r == Tree::TreeDeath)
        return; // do nothing atm
    if (r==Tree::TreeHarvest) {
        // regular harvest
        mHarvested +=removed_volume;
    }
    if (r==Tree::TreeDisturbance) {
        // if we have an active salvage activity, then store
        if (mSTP->salvageActivity()) {
            if (mSTP->salvageActivity()->testRemove(tree)) {
                mDisturbed += removed_volume;
            }
        }

    }
}


void FMStand::sleep(int years_to_sleep)
{
    mYearsToWait = qMax(mYearsToWait, qMax(years_to_sleep,0));
}


double FMStand::calculateMAI()
{
    // MAI: delta standing volume + removed volume, per year
    // removed volume: mortality, management, disturbances
    if (mRemovedVolumeTicks==0)
        return mMAIdecade;
    mMAIdecade = ((mVolume - mLastMAIVolume) + mRemovedVolumeDecade) / double(mRemovedVolumeTicks);
    mMAItotal = (mVolume + mRemovedVolumeTotal) / absoluteAge();
    mLastMAIVolume = mVolume;
    // reset counters
    mRemovedVolumeDecade = 0.;
    mRemovedVolumeTicks = 0;
    return mMAIdecade;
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

QStringList FMStand::info()
{
    QStringList lines;
    lines << QString("id: %1").arg(id())
          << QString("unit: %1").arg(unit()->id());
    lines  << "-" << unit()->info() << "/-"; // sub sections
    if (currentActivity()) {
        lines << QString("activity: %1").arg(currentActivity()->name()) << "-" << currentActivity()->info();
        // activity properties
        lines << QString("active: %1").arg(currentFlags().active());
        lines << QString("enabled: %1").arg(currentFlags().enabled());
        lines << QString("simulate: %1").arg(currentFlags().isDoSimulate());
        lines << QString("execute immediate: %1").arg(currentFlags().isExecuteImmediate());
        lines << QString("final harvest: %1").arg(currentFlags().isFinalHarvest());
        lines << QString("use scheduler: %1").arg(currentFlags().isScheduled());
        lines << QString("in scheduler: %1").arg(currentFlags().isPending());
        lines <<  "/-";
    }
    lines << QString("agent: %1").arg(unit()->agent()->type()->name());
    lines << QString("last update: %1").arg(lastUpdate());
    lines << QString("sleep (years): %1").arg(sleepYears());
    lines << QString("scheduled harvest: %1").arg(scheduledHarvest());
    lines << QString("basal area: %1").arg(basalArea());
    lines << QString("volume: %1").arg(volume());
    lines << QString("age: %1").arg(age());
    lines << QString("absolute age: %1").arg(absoluteAge());
    lines << QString("N/ha: %1").arg(stems());
    lines << QString("MAI (decadal) m3/ha*yr: %1").arg(meanAnnualIncrement());
    lines << "Basal area per species";
    for (int i=0;i<nspecies();++i) {
        lines << QString("%1: %2").arg(speciesData(i).species->id()).arg(speciesData(i).basalArea);
    }

    lines  << "All activities" << "-";
    for (QVector<ActivityFlags>::const_iterator it = mStandFlags.constBegin(); it!=mStandFlags.constEnd(); ++it) {
        const ActivityFlags &a = *it;
        lines << QString("%1 (active): %2").arg(a.activity()->name()).arg(a.active())
                 << QString("%1 (enabled): %2").arg(a.activity()->name()).arg(a.enabled());
    }
    lines << "/-";


    // stand properties
    if (mStandPropertyStorage.contains(this)) {
        QHash<QString, QJSValue> &props = mStandPropertyStorage[this];
        lines << QString("properties: %1").arg(props.size()) << "-";
        QHash<QString, QJSValue>::const_iterator i = props.constBegin();
        while (i != props.constEnd()) {
            lines << QString("%1: %2").arg(i.key()).arg(i.value().toString());
            ++i;
        }
        lines << "/-";
    }

    // scheduler info
    lines << unit()->constScheduler()->info(id());

    return lines;
}

void FMStand::newRotatation()
{
    mRotationStartYear = ForestManagementEngine::instance()->currentYear(); // reset stand age to 0.
    mRemovedVolumeTotal = 0.;
    mRemovedVolumeDecade = 0.;
    mRemovedVolumeTicks = 0;
    mLastMAIVolume = 0.;
    mMAIdecade = 0.;
    mMAItotal = 0.;
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
