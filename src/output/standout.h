#ifndef STANDOUT_H
#define STANDOUT_H
#include "output.h"

class StandOut : public Output
{
public:
    StandOut();
    virtual void exec();
    virtual void setup();
};

#endif // STANDOUT_H
