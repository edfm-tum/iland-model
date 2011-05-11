#ifndef CARBONFLOWOUT_H
#define CARBONFLOWOUT_H
#include "output.h"

class CarbonFlowOut : public Output
{
public:
    CarbonFlowOut();
    virtual void exec();
    virtual void setup();

};

#endif // CARBONFLOWOUT_H
