#include "global.h"
#include "fome_global.h"
#include "fmstp.h"
#include "fmstand.h"
FMSTP::FMSTP()
{
}

// read the setting from the setup-javascript object
void FMSTP::setup(QJSValue &js_value)
{
    QJSValue thinning = valueFromJs(js_value, "thinning", "", "setup STP");
    Schedule thinning_timing;
    thinning_timing.setup(valueFromJs(thinning, "timing", "", "setup thinning"));
    qDebug() << thinning_timing.dump();
}

// run the management for the forest stand 'stand'
bool FMSTP::execute(FMStand &stand)
{
    switch (stand.phase()) {
        case Regeneration: break;
    }
    return true;
}

QJSValue FMSTP::valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage)
{
   if (!js_value.hasOwnProperty(key)) {
       if (!errorMessage.isEmpty())
           throw IException(QString("Error: required key '%1' not found. In: %2").arg(key).arg(errorMessage));
       else
           return default_value;
   }
   return js_value.property(key);
}



void FMSTP::Schedule::setup(QJSValue &js_value)
{
    clear();
    if (js_value.isObject()) {
        tmin = valueFromJs(js_value, "min", "-1").toInt();
        tmax = valueFromJs(js_value, "max", "-1").toInt();
        topt = valueFromJs(js_value, "opt", "-1").toInt();
        tminrel = valueFromJs(js_value, "minRel", "-1").toNumber();
        tmaxrel = valueFromJs(js_value, "maxRel", "-1").toNumber();
        toptrel = valueFromJs(js_value, "optRel", "-1").toNumber();
        force_execution = valueFromJs(js_value, "force", "false").toBool();
    } else if (js_value.isNumber()) {
        topt = js_value.toNumber();
    } else {
        throw IException(QString("Error in setting up schedule/timing. Invalid javascript object: %1").arg(js_value.toString()));
    }
}

QString FMSTP::Schedule::dump() const
{
    return QString("schedule. tmin/topt/tmax: %1/%2/%3 relative: min/opt/max: %4/%5/%6 force: &7").arg(tmin).arg(topt).arg(tmax)
                                                                                                  .arg(tminrel).arg(toptrel).arg(tmaxrel).arg(force_execution);
}
