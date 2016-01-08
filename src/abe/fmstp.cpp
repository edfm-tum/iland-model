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

#include "global.h"
#include "abe_global.h"
#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fomewrapper.h"

#include "expression.h"


namespace ABE {

/** @class FMSTP
    @ingroup abe
    The FMSTP class encapsulates a stand treatment program, which is defined in Javascript.

  */

// static values
bool FMSTP::mVerbose = false;

FMSTP::FMSTP()
{
    mSalvage = 0;
    mRotationLength[0] = 90.; // sensible defaults
    mRotationLength[1] = 100.;
    mRotationLength[2] = 110.;
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
        mActivityStand.push_back(mActivities.at(i)->standFlags()); // stand = null: create a copy of the activities' base flags
        mActivities.at(i)->setIndex(i);
        if (mActivities.at(i)->isRepeatingActivity())
            mHasRepeatingActivities = true;
        if (mActivities.at(i)->standFlags().isSalvage()) {
            mSalvage = dynamic_cast<ActSalvage*>(mActivities.at(i));
            mHasRepeatingActivities = false;
        }
    }

    // (3) set up top-level events
    mEvents.setup(js_value, QStringList() << QStringLiteral("onInit") << QStringLiteral("onExit"));
}

bool FMSTP::executeRepeatingActivities(FMStand *stand)
{
    if (mSalvage)
        if (stand->totalHarvest() || stand->property("_run_salvage").toBool()) {
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

void FMSTP::evaluateDynamicExpressions(FMStand *stand)
{
    foreach(Activity *act, mActivities)
        act->evaluateDyanamicExpressions(stand);
}

// read the setting from the setup-javascript object
void FMSTP::internalSetup(const QJSValue &js_value, int level)
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
            // parse special properties
            if (it.name()=="U" && it.value().isArray()) {
                QVariantList list = it.value().toVariant().toList();
                if (list.length()!=3)
                    throw IException("STP: the 'U'-property needs to be an array with three elements!");
                for (int i=0;i<list.length();++i)
                    mRotationLength[i] = list.at(i).toInt();
                continue;
            }
            if (it.name()=="options") {
                mOptions = it.value();
                continue;
            }
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

void FMSTP::setupActivity(const QJSValue &js_value, const QString &name)
{
    QString type = js_value.property("type").toString();
    if (verbose())
        qCDebug(abeSetup) << "setting up activity of type" << type << "from JS";
    Activity *act = Activity::createActivity(type, this);
    if (!act) return; // actually, an error is thrown in the previous call.

    // use id-property if available, or the object-name otherwise
    act->setName(valueFromJs(js_value, "id", name).toString());
    // call the setup routine (overloaded version)
    act->setup(js_value);

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
           throw IException(QString("Error: required key '%1' not found. In: %2 (JS: %3)").arg(key).arg(errorMessage).arg(FomeScript::JStoString(js_value)));
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
            throw IException(QString("Error: required key '%1' not found. In: %2 (JS: %3)").arg(key).arg(errorMessage).arg(FomeScript::JStoString(js_value)));
        else
            return default_bool_value;
    }
    return js_value.property(key).toBool();

}

bool FMSTP::checkObjectProperties(const QJSValue &js_value, const QStringList &allowed_properties, const QString &errorMessage)
{
    QJSValueIterator it(js_value);
    bool found_issues = false;
    while (it.hasNext()) {
        it.next();
        if (!allowed_properties.contains(it.name()) && it.name()!=QLatin1String("length")) {
            qCDebug(abe) << "Syntax-warning: The javascript property" << it.name() << "is not used! In:" << errorMessage;
            found_issues = true;
        }
    }
    return !found_issues;
}

} // namespace
