#ifndef SVDOUT_H
#define SVDOUT_H
#include "output.h"


class SVDGPPOut: public Output
{
public:
    SVDGPPOut();
    virtual void exec();
    virtual void setup();
private:
    QStringList mSpeciesList;
    int mSpeciesIndex[10];
};

#endif // SVDOUT_H