#ifndef CARBONOUT_H
#define CARBONOUT_H
#include "output.h"

#include "firemodule.h"

class FireOut: public Output
{
public:
    FireOut();
    void setFireModule(FireModule *module) { mFire = module; }
    virtual void exec();
    virtual void setup();
private:
    FireModule *mFire;
};

#endif // CARBONOUT_H
