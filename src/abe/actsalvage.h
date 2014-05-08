#ifndef ACTSALVAGE_H
#define ACTSALVAGE_H

#include "activity.h"

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
    bool testRemove(Tree* tree) const;
private:
    Expression *mCondition; ///< formula to determine which trees should be harvested
    int mMaxPreponeActivity; ///< no of years that a already scheduled (regular) activity is 'preponed'

};


} // namespace
#endif // ACTSALVAGE_H
