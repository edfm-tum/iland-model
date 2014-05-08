#include "global.h"
#include "abe_global.h"
#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fomewrapper.h"

#include "expression.h"


namespace ABE {
// static values
bool FMSTP::mVerbose = false;

FMSTP::FMSTP()
{
    mSalvage = 0;
}

FMSTP::~FMSTP()
{
    clear();
}

Activity *FMSTP::activity(const QString &name) const
{
    int idx = mActivityNames.indexOf(name);
    if (idx==-1)
        return 0;
    return mActivities[idx];
}

bool activityScheduledEarlier(const Activity *a, const Activity *b)
{
    return a->earlistSchedule() < b->earlistSchedule();
}

void FMSTP::setup(QJSValue &js_value, const QString name)
{
    if(!name.isEmpty())
        mName = name;

    // (1) scan recursively the data structure and create
    //     all activites
    internalSetup(js_value, 0);

    // (2) create all other required meta information (such as ActivityStand)
    // sort activites based on the minimum execution time
    std::sort(mActivities.begin(), mActivities.end(), activityScheduledEarlier);

    mActivityNames.clear();
    mHasRepeatingActivities = false;
    for (int i=0;i<mActivities.count();++i) {
        mActivityNames.push_back(mActivities.at(i)->name());
        mActivityStand.push_back(mActivities.at(i)->standFlags(0)); // stand = 0: create a copy of the activities' base flags
        mActivities.at(i)->setIndex(i);
        if (mActivities.at(i)->isRepeatingActivity())
            mHasRepeatingActivities = true;
        if (mActivities.at(i)->standFlags(0).isSalvage()) {
            mSalvage = dynamic_cast<ActSalvage*>(mActivities.at(i));
            mHasRepeatingActivities = false;
        }
    }

    // (3) set up top-level events
    mEvents.setup(js_value, QStringList() << QStringLiteral("onInit") << QStringLiteral("onExit"));
}

bool FMSTP::executeRepeatingActivities(FMStand *stand)
{
    if (mSalvage && stand->totalHarvest()) {
        // at this point totalHarvest is only disturbance related harvests.
        stand->executeActivity(mSalvage);
    }
    if (!mHasRepeatingActivities)
        return false;
    bool result = false;
    for (int i=0;i<mActivities.count();++i)
        if (mActivities.at(i)->schedule().repeat) {
            if (!stand->flags(i).active() || !stand->flags(i).enabled())
                continue;
            if (stand->trace())
                qCDebug(abe) << "running repeating activity" << mActivities.at(i)->name();
            result |= stand->executeActivity(mActivities[i]);
        }
    return result; // return true if at least one repeating activity was executed.

}

// read the setting from the setup-javascript object
void FMSTP::internalSetup(QJSValue &js_value, int level)
{

    // top-level
    if (js_value.hasOwnProperty("schedule")) {
        setupActivity(js_value, "unnamed");
        return;
    }

    // nested objects
    if (js_value.isObject()) {
        QJSValueIterator it(js_value);
        while (it.hasNext()) {
            it.next();
            if (it.value().hasOwnProperty("type")) {
                // try to set up as activity
                setupActivity(it.value(), it.name());
            } else if (it.value().isObject() && !it.value().isCallable()) {
                // try to go one level deeper
                if (FMSTP::verbose())
                    qCDebug(abeSetup) << "entering" << it.name();
                if (level<10)
                    internalSetup(it.value(), ++level);
                else
                    throw IException("setup of STP: too many nested levels (>=10) - check your syntax!");
            }
        }
    } else {
        qCDebug(abeSetup) << "FMSTP::setup: not a valid javascript object.";
    }
}


void FMSTP::dumpInfo()
{
    if (!abe().isDebugEnabled())
        return;
    qCDebug(abe) << " ***************************************";
    qCDebug(abe) << " **************** Program dump for:" << name();
    qCDebug(abe) << " ***************************************";
    foreach(Activity *act, mActivities) {
        qCDebug(abe) << "******* Activity *********";
        QString info =  act->info().join('\n');
        qCDebug(abe) << info;

    }
}

void FMSTP::setupActivity(QJSValue &js_value, const QString &name)
{
    QString type = js_value.property("type").toString();
    if (verbose())
        qCDebug(abeSetup) << "setting up activity of type" << type << "from JS";
    Activity *act = Activity::createActivity(type, this);
    if (!act) return; // actually, an error is thrown in the previous call.

    // call the setup routine (overloaded version)
    act->setup(js_value);
    // use id-property if available, or the object-name otherwise
    act->setName(valueFromJs(js_value, "id", name).toString());

    // call the onCreate handler:
    FomeScript::bridge()->setActivity(act);
    act->events().run(QStringLiteral("onCreate"),0);
    mActivities.push_back(act);
}

void FMSTP::clear()
{
    qDeleteAll(mActivities);
    mActivities.clear();
}

QJSValue FMSTP::valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage)
{
   if (!js_value.hasOwnProperty(key)) {
       if (!errorMessage.isEmpty())
           throw IException(QString("Error: required key '%1' not found. In: %2 (JS: %3)").arg(key).arg(errorMessage).arg(js_value.toString()));
       else if (default_value.isEmpty())
           return QJSValue();
       else
           return default_value;
   }
   return js_value.property(key);
}

bool FMSTP::boolValueFromJs(const QJSValue &js_value, const QString &key, const bool default_bool_value, const QString &errorMessage)
{
    if (!js_value.hasOwnProperty(key)) {
        if (!errorMessage.isEmpty())
            throw IException(QString("Error: required key '%1' not found. In: %2 (JS: %3)").arg(key).arg(errorMessage).arg(js_value.toString()));
        else
            return default_bool_value;
    }
    return js_value.property(key).toBool();

}

} // namespace
