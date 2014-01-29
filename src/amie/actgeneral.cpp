#include "global.h"
#include "amie_global.h"
#include "actgeneral.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"

namespace AMIE {


QStringList ActGeneral::info()
{
    QStringList lines = Activity::info();
    lines << "this is the 'general' activity";
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
    if (FMSTP::verbose())
        qDebug() << "activity 'general': execute";
    QJSValue result = mAction.call();
    if (result.isError()) {
        throw IException(QString("Javascript error in 'general' activity: %2").arg(result.toString()));
    }
    return result.toBool();
}


} // namespace
