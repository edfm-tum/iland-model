#include "global.h"
#include "abe_global.h"
#include "actgeneral.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"

namespace ABE {


QStringList ActGeneral::info()
{
    QStringList lines = Activity::info();
    lines << "this is the 'general' activity; the activity is not scheduled. Use the action-slot to define what should happen.";
    return lines;
}

void ActGeneral::setup(QJSValue value)
{
    Activity::setup(value);
    // specific
    mAction = FMSTP::valueFromJs(value, "action", "", "Activity of type 'general'.");
    if (!mAction.isCallable())
        throw IException("'general' activity has not a callable javascript 'action'.");
}

bool ActGeneral::execute(FMStand *stand)
{
    FomeScript::setExecutionContext(stand);
    if (FMSTP::verbose() || stand->trace())
        qCDebug(amie) << stand->context() << "activity 'general': execute of" << name();

    QJSValue result = mAction.call();
    if (result.isError()) {
        throw IException(QString("%1 Javascript error in 'general' activity '%3': %2").arg(stand->context()).arg(result.toString()).arg(name()));
    }
    return result.toBool();
}


} // namespace
