#ifndef ACTSALVAGE_H
#define ACTSALVAGE_H

#include "activity.h"
#include "grid.h"
class Expression; // forward
class Tree; // forward
namespace ABE {

class FMSTP; // forward
class FMStand; // forward

class ActSalvage : public Activity
{
public:
    ActSalvage(FMSTP *parent);
    ~ActSalvage();
    QString type() const { return "salvage"; }
    void setup(QJSValue value);
    bool execute(FMStand *stand);
    QStringList info();
    // special functions of salvage activity

    /// return true, if the (disturbed) tree should be harvested by the salvage activity
    bool evaluateRemove(Tree* tree) const;
private:
    void checkStandAfterDisturbance(FMStand *stand);
    int floodFillHelper(Grid<int> &grid, QPoint start, int color);
    bool mDebugSplit;
    Expression *mCondition; ///< formula to determine which trees should be harvested
    int mMaxPreponeActivity; ///< no of years that a already scheduled (regular) activity is 'preponed'
    double mThresholdTotal; ///< threshold (relative disturbend volume) for total disturbance
    double mThresholdMinimal; ///< lower threshold (below no action is taken)

};


} // namespace
#endif // ACTSALVAGE_H
