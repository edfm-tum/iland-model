#ifndef UNDERSTORYOUT_H
#define UNDERSTORYOUT_H

#include "output.h"
#include "expression.h"

class UnderstoryOut : public Output
{
public:
    UnderstoryOut();
    virtual void exec();
    virtual void setup();
private:
    Expression mCondition; // condition for landscape-level output
    Expression mConditionDetails; // condition for resource-unit-level output

};

#endif // UNDERSTORYOUT_H
