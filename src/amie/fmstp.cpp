#include "global.h"
#include "amie_global.h"
#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fomewrapper.h"

#include "expression.h"

// activity types
#include "actgeneral.h"

namespace AMIE {
// static values
bool FMSTP::mVerbose = false;

FMSTP::FMSTP()
{
}

FMSTP::~FMSTP()
{
    clear();
}

// read the setting from the setup-javascript object
void FMSTP::setup(QJSValue &js_value, const QString name, int level)
{
    if(!name.isEmpty())
        mName = name;

    // top-level
    if (js_value.hasOwnProperty("schedule")) {
        setupActivity(js_value);
        return;
    }

    // nested objects
    if (js_value.isObject()) {
        QJSValueIterator it(js_value);
        while (it.hasNext()) {
            it.next();
            if (it.value().hasOwnProperty("schedule")) {
                // set up as activity
                setupActivity(it.value());
            } else if (it.value().isObject() && !it.value().isCallable()) {
                // try to go one level deeper
                if (FMSTP::verbose())
                    qDebug() << "entering" << it.name();
                if (level<10)
                    setup(it.value(), QString(), ++level);
            }
        }
    } else {
        qDebug() << "FMSTP::setup: not a valid javascript object.";
    }

    // tests
//    FMStand *test = new FMStand(0,1);
//    thinning_constraints.evaluate(test);
//    for (int i=0;i<100;++i) {
//        test->reload();
//        qDebug()<< "year" << test->age() << "result: " << thinning_timing.value(test);
//    }
//    // testing events
//    qDebug() << "onEnter: " << thinning_events.run("onEnter", test);
//    qDebug() << "onExit: " << thinning_events.run("onExit", test);

//    delete test;
}

// run the management for the forest stand 'stand'
bool FMSTP::execute(FMStand &stand)
{
    switch (stand.phase()) {
        case Regeneration: break;
    }
    return true;
}

void FMSTP::dumpInfo()
{
    foreach(Activity *act, mActivities) {
        qDebug() << "******* Activity *********";
        QStringList info =  act->info();
        foreach(const QString &s, info)
            qDebug() << s;
    }
}

void FMSTP::setupActivity(QJSValue &js_value)
{
    QString type = js_value.property("type").toString();
    if (verbose())
        qDebug() << "setting up activity of type" << type << "from JS";
    Activity *act = 0;
    if (type=="general")
        act = new ActGeneral(this);

    if (!act) {
        throw IException(QString("Error: the activity type '%1' is not a valid type.").arg(type));
    }

    // call the setup routine (overloaded version)
    act->setup(js_value);
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
           throw IException(QString("Error: required key '%1' not found. In: %2").arg(key).arg(errorMessage));
       else
           return default_value;
   }
   return js_value.property(key);
}

} // namespace
