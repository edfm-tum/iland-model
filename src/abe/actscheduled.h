#ifndef ACTSCHEDULED_H
#define ACTSCHEDULED_H
#include "activity.h"

namespace ABE {

class FMStand; // forward
class FMSTP; // forward

class ActScheduled : public Activity
{
public:
    ActScheduled(FMSTP *parent);

    // Activity interface
public:
    QString type() const { return "scheduled"; }
    void setup(QJSValue value);
    bool execute(FMStand *stand);
    bool evaluate(FMStand *stand);
    QStringList info();
};

} // namespace
#endif // ACTSCHEDULED_H
