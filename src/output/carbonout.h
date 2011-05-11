#ifndef CARBONOUT_H
#define CARBONOUT_H
#include "output.h"

class CarbonOut: public Output
{
public:
    CarbonOut();
    virtual void exec();
    virtual void setup();
};

#endif // CARBONOUT_H
