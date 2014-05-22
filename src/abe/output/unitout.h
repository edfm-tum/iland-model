#ifndef UNITOUT_H
#define UNITOUT_H
#include "output.h"

namespace ABE {

class UnitOut : public Output
{
public:
    UnitOut();
    virtual void exec();
    virtual void setup();

};


} // namespace
#endif // UNITOUT_H
