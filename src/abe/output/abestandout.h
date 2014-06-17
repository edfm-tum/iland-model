#ifndef ABESTANDOUT_H
#define ABESTANDOUT_H
#include "output.h"

namespace ABE {

class ABEStandOut : public Output
{
public:
    ABEStandOut();
    virtual void exec();
    virtual void setup();

};


} // namespace

#endif // ABESTANDOUT_H
