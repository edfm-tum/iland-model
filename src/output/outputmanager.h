#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include "output.h"

class OutputManager
{
public:
    OutputManager(); ///< create all outputs
    ~OutputManager();
    void setup();
private:
    QList<Output*> mOutputs; ///< list of outputs in system
};

#endif // OUTPUTMANAGER_H
