#ifndef STANDOUT_H
#define STANDOUT_H
#include "output.h"
#include "expression.h"

/** StandOut is basic stand level info per species and ressource unit */
class StandOut : public Output
{
public:
    StandOut();
    virtual void exec();
    virtual void setup();
};



#endif // STANDOUT_H
