#ifndef ACTPLANTING_H
#define ACTPLANTING_H

#include "activity.h"
#include "species.h"

namespace ABE {

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
        SPlantingItem(): species(0), fraction(0.), height(0.05), clear(false), grouped(false), group_type(-1), group_random_count(-1), offset(0), spacing(0) {}
        Species *species;
        double fraction;
        double height;
        bool clear;
        bool grouped; ///< true for pattern creation
        int group_type; ///< index of the pattern in the pattern list
        int group_random_count; ///< if >0: number of random patterns
        int offset; ///< offset (in LIF-pixels) for the pattern algorithm
        int spacing;  ///< distance between two applications of a pattern
        bool setup(QJSValue value);
    };
    QVector<SPlantingItem> mItems;
    bool mRequireLoading;


};

} // end namespace
#endif // ACTPLANTING_H
