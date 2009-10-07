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
    void save(); ///< save transactions of all outputs
    void close(); ///< close all outputs
private:
    QList<Output*> mOutputs; ///< list of outputs in system
    // transactions
    void startTransaction(); ///< start database transaction  (if output database is open, i.e. >0 DB outputs are active)
    void endTransaction(); ///< ends database transaction
    bool mTransactionOpen; ///< for database outputs: if true, currently a transaction is open
};

#endif // OUTPUTMANAGER_H
