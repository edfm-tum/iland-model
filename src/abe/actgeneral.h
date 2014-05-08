#ifndef ACTGENERAL_H
#define ACTGENERAL_H
#include "activity.h"

#include <QJSValue>
namespace ABE {
class FMStand; // forward
class FMSTP; // forward

class ActGeneral : public Activity
{
public:
    ActGeneral(FMSTP* parent): Activity(parent) {}
    QString type() const { return "general"; }
    QStringList info();
    void setup(QJSValue value);
    bool execute(FMStand *stand);
private:
    QJSValue mAction;
};

} // namespace
#endif // ACTGENERAL_H
