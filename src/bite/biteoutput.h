#ifndef BITEOUTPUT_H
#define BITEOUTPUT_H
#include "output.h"

namespace BITE {

class BiteOutput: public Output
{
public:
    BiteOutput();
    virtual void exec();
    virtual void setup();

};

} // namespace

#endif // BITEOUTPUT_H
