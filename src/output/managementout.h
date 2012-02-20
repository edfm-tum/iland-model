#ifndef MANAGEMENTOUT_H
#define MANAGEMENTOUT_H
#include "output.h"

class ManagementOut: public Output
{
public:
    ManagementOut();
    virtual void exec();
    virtual void setup();
};

#endif // MANAGEMENTOUT_H
