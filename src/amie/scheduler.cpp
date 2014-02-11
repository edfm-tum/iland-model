#include "amie_global.h"
#include "scheduler.h"

#include "fmstand.h"
#include "activity.h"
#include "fmunit.h"
#include "fmstp.h"

namespace AMIE {

Scheduler::Scheduler()
{
}

void Scheduler::addTicket(FMStand *stand, ActivityFlags *flags)
{
    // for the time being, force execution
    if (FMSTP::verbose())
        qDebug()<< "ticked added for stand" << stand->id();

    flags->activity()->execute(stand);
    flags->setIsPending(false);
    flags->setActive(false); // done; TODO: check for repeating activities
    stand->afterExecution();
}


} // namespace
