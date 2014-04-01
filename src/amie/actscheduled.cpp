#include "amie_global.h"
#include "actscheduled.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fmtreelist.h"

namespace AMIE {




ActScheduled::ActScheduled(FMSTP *parent): Activity(parent)
{
    mBaseActivity.setIsScheduled(true); // use the scheduler
    mBaseActivity.setDoSimulate(true); // simulate per default
}

void ActScheduled::setup(QJSValue value)
{
    Activity::setup(value);
    events().setup(value, QStringList() << "onEvaluate");

    if (!events().hasEvent(QStringLiteral("onEvaluate")))
        throw IException("activity %1 (of type 'scheduled') requires to have the 'onSchedule' event.");

}

bool ActScheduled::execute(FMStand *stand)
{
    if (events().hasEvent(QStringLiteral("onExecute"))) {
        // switch off simulation mode
        stand->currentFlags().setDoSimulate(false);
        // execute this event
        bool result =  Activity::execute(stand);
        stand->currentFlags().setDoSimulate(true);
        return result;
    } else {
        // default behavior: process all marked trees (harvest / cut)
        if (stand->trace()) qCDebug(abe) << stand->context() << "activity" << name() << "remove all marked trees.";
        FMTreeList trees(stand);
        trees.removeMarkedTrees();
        return true;
    }
}

bool ActScheduled::evaluate(FMStand *stand)
{
    // this is called when it should be tested
    stand->currentFlags().setDoSimulate(true);
    QString result = events().run(QStringLiteral("onEvaluate"), stand);
    if (stand->trace())
        qCDebug(abe) << stand->context() << "executed onSchedule event of" << name() << "with result:" << result;
    bool ok;
    double harvest = result.toDouble(&ok);
    if (ok) {
        // the return value is interpreted as scheduled harvest; if this value is 0, then no
        if (harvest==0.)
            return false;
        stand->addScheduledHarvest(harvest);
        if (stand->trace())
            qCDebug(abe) << stand->context() << "scheduled harvest is now" << stand->scheduledHarvest();
        return true;
    }
    bool bool_result = result == "true";
    return bool_result;
}

QStringList ActScheduled::info()
{
    QStringList lines = Activity::info();
    lines << "this is an activity of type 'scheduled'.";
    return lines;
}


} // namespace
