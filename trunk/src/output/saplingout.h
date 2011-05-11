#ifndef SAPLINGOUT_H
#define SAPLINGOUT_H
#include "output.h"

class SaplingOut : public Output
{
public:
    SaplingOut();
    virtual void exec();
    virtual void setup();
};

#endif // SAPLINGOUT_H
