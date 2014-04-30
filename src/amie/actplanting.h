#ifndef ACTPLANTING_H
#define ACTPLANTING_H

#include "activity.h"
#include "species.h"

namespace AMIE {

class FMSTP; // forward
class FMStand; // forward

class ActPlanting : public Activity
{
public:
    ActPlanting(FMSTP *parent);
    QString type() const { return "planting"; }
    void setup(QJSValue value);
    bool execute(FMStand *stand);
    //bool evaluate(FMStand *stand);
    QStringList info();
private:
    struct SPlantingItem {
        SPlantingItem(): species(0), fraction(0.), clear(false) {}
        Species *species;
        double fraction;
        bool clear;
        bool setup(QJSValue value);
    };
    QVector<SPlantingItem> mItems;
};

} // end namespace
#endif // ACTPLANTING_H
