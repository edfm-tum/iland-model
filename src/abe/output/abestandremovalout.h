#ifndef ABESTANDREMOVALOUT_H
#define ABESTANDREMOVALOUT_H
#include "output.h"

namespace ABE {

class ABEStandRemovalOut : public Output
{
public:
    ABEStandRemovalOut();
    virtual void exec();
    virtual void setup();

};


} // namespace

#endif // ABESTANDREMOVALOUT_H
