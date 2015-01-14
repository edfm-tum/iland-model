#ifndef ABESTANDOUT_H
#define ABESTANDOUT_H
#include "output.h"
#include "expression.h"

namespace ABE {

class ABEStandOut : public Output
{
public:
    ABEStandOut();
    virtual void exec();
    virtual void setup();
private:
    Expression mCondition;

};


} // namespace

#endif // ABESTANDOUT_H
