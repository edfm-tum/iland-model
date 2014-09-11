#include "agent.h"
#include "agenttype.h"
#include "scheduler.h"
#include "forestmanagementengine.h"
#include "fmstand.h"
#include "fomescript.h"

namespace ABE {

int Agent::mAgentsCreated = 0;

Agent::Agent(AgentType *type, QJSValue js)
{
    mType = type;
    mJSAgent = js;
    mAgentsCreated++;
    mName = QString("agent_%1").arg(mAgentsCreated);
}

void Agent::setName(const QString &name)
{
    mName = name;
    mJSAgent.setProperty("name", name);
}

double Agent::useSustainableHarvest() const
{
    return schedulerOptions().useSustainableHarvest;
}

void Agent::setup()
{
    QJSValue scheduler = jsAgent().property("scheduler");
    mSchedulerOptions.setup( scheduler );

    FMSTP *stp = type()->stpByName("default");
    if (!stp)
        throw IException("Agent::setup(): default-STP not defined");

    QJSValue onSelect_handler = type()->jsObject().property("onSelect");

    const QMultiMap<FMUnit*, FMStand*> &stand_map = ForestManagementEngine::instance()->stands();
    foreach (FMUnit *unit, mUnits) {
        QMultiMap<FMUnit*, FMStand*>::const_iterator it = stand_map.constFind(unit);
        while (it!=stand_map.constEnd() && it.key()==unit) {
            FMStand *stand = it.value();
            // check if STP is already assigned. If not, do it now.
            if (!stand->stp()) {
                stand->reload(); // fetch data from iLand ...
                if (onSelect_handler.isCallable()) {
                    FomeScript::setExecutionContext(stand);
                    //QJSValue mix = onSelect_handler.call();
                    QJSValue mix = onSelect_handler.callWithInstance(type()->jsObject());
                    QString mixture_type = mix.toString();
                    if (!type()->stpByName(mixture_type))
                        throw IException(QString("Agent::setup(): the selected mixture type '%1' for stand '%2' is not valid for agent '%3'.").arg(mixture_type).arg(stand->id()).arg(mName));
                    stand->setSTP(type()->stpByName(mixture_type));
                } else {
                    // todo.... some automatic stp selection
                    stand->setSTP(stp);
                }
                stand->setU( unit->U() );
                stand->setThinningIntensity( unit->thinningIntensity() );
                stand->setTargetSpeciesIndex( unit->targetSpeciesIndex() );
                stand->initialize(); // run initialization

            }
            ++it;
        }
    }
}

} // namespace
