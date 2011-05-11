#ifndef STANDDEADOUT_H
#define STANDDEADOUT_H
#include "output.h"

class StandDeadOut : public Output
{
public:
    StandDeadOut();
    virtual void exec();
    virtual void setup();
};

#endif // STANDDEADOUT_H
