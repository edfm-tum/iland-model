#include "amie_global.h"
#include "scheduler.h"

#include "fmstand.h"
#include "activity.h"
#include "fmunit.h"

namespace AMIE {

Scheduler::Scheduler()
{
}

void Scheduler::addTicket(FMStand *stand, ActivityFlags *flags)
{
    // for the time being, force execution
    flags->activity()->execute(stand);
    flags->setIsPending(false);
    stand->afterExecution();
}


} // namespace
