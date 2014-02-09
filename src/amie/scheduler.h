#ifndef SCHEDULER_H
#define SCHEDULER_H



namespace AMIE {
class FMStand; // forward
class ActivityFlags; // forward
class Scheduler
{
public:
    Scheduler();
    void addTicket(FMStand *stand, ActivityFlags *flags);
};



} // namespace
#endif // SCHEDULER_H
