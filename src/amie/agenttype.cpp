#include "global.h"
#include "amie_global.h"
#include "agenttype.h"
#include "fmstp.h"
#include "forestmanagementengine.h"
#include "fmunit.h"
#include "fmstand.h"
#include "fomescript.h"

#include <QJSEngine>
namespace AMIE {


AgentType::AgentType()
{
}

void AgentType::setupSTP(const QString agent_name)
{
    mName = agent_name;
    mSTP.clear();
    mUnits.clear();
    QJSValue agent = GlobalSettings::instance()->scriptEngine()->globalObject().property(agent_name);
    if (!agent.isObject())
        throw IException(QString("AMIE:AgentType:setup: the javascript object for agent '%1' could not be found.").arg(agent_name));
    QJSValue stps = agent.property("stp");
    if (!stps.isObject())
        throw IException(QString("AMIE:AgentType:setup: the javascript definition of agent '%1' does not have a section for 'stp'.").arg(agent_name));
    QJSValueIterator it(stps);
    while (it.hasNext()) {
        it.next();
        FMSTP *stp = ForestManagementEngine::instance()->stp(it.value().toString());
        if (!stp)
           throw IException(QString("AMIE:AgentType:setup: definition of agent '%1': the STP for mixture type '%2': '%3' is not available.").arg(agent_name).arg(it.name()).arg(it.value().toString()));
        mSTP[it.name()] = stp;
    }

    if (FMSTP::verbose())
        qDebug() << "setup of agent" << agent_name << mSTP.size() << "links to STPs established.";

}

void AgentType::setup()
{
    FMSTP *stp = mSTP.value("default", 0);
    if (!stp)
        throw IException("AgentType::setup(): default-STP not defined");

    QJSValue onSelect_handler = GlobalSettings::instance()->scriptEngine()->globalObject().property(QString("%1.onSelect").arg(mName));

    const QMultiMap<FMUnit*, FMStand*> &stand_map = ForestManagementEngine::instance()->stands();
    foreach (FMUnit *unit, mUnits) {
        QMultiMap<FMUnit*, FMStand*>::const_iterator it = stand_map.constFind(unit);
        while (it!=stand_map.constEnd() && it.key()==unit) {
            FMStand *stand = it.value();
            stand->reload(); // fetch data from iLand ...
            if (onSelect_handler.isCallable()) {
                FomeScript::setExecutionContext(stand);
                QJSValue mix = onSelect_handler.call();
                QString mixture_type = mix.toString();
                if (!mSTP.contains(mixture_type))
                    throw IException(QString("AgentType::setup(): the selected mixture type '%1' for stand '%2' is not valid for agent '%3'.").arg(mixture_type).arg(stand->id()).arg(mName));
                stand->initialize(mSTP[mixture_type]);
            } else {
                // todo.... some automatic stp selection

            }
            stand->initialize(stp);
            stand->afterExecution(); // find out the next activity

            ++it;
        }
    }

}


} // namespace
