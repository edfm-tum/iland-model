#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include "output.h"

class OutputManager
{
public:
    OutputManager(); ///< create all outputs
    ~OutputManager();
    void setup(); ///< setup of the outputs + switch on/off (from project file)
    Output *find(const QString& tableName); ///< search for output and return pointer, NULL otherwise
    bool execute(const QString& tableName); ///< execute output with a given name. returns true if executed.
private:
    QList<Output*> mOutputs; ///< list of outputs in system
};

#endif // OUTPUTMANAGER_H
