#ifndef WATEROUT_H
#define WATEROUT_H
#include "output.h"
#include "expression.h"


class WaterOut : public Output
{
public:
    WaterOut();
    virtual void exec();
    virtual void setup();
private:
    Expression mCondition; // condition for landscape-level output
    Expression mConditionDetails; // condition for resource-unit-level output

};

#endif // WATEROUT_H
