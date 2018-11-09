#ifndef BITELIFECYCLE_H
#define BITELIFECYCLE_H

#include "biteitem.h"
#include "bitecellscript.h"

namespace BITE {

class BiteLifeCycle: public BiteItem
{
    Q_OBJECT

public:
    Q_INVOKABLE BiteLifeCycle(QJSValue obj);

    void setup(BiteAgent *parent_agent);
    QString info();
    void notify(BiteCell *cell, BiteCell::ENotification what);


    bool dieAfterDispersal() const { return mDieAfterDispersal; }

    /// fetch the number of cycles the agent should run for the cell
    int numberAnnualCycles(BiteCell *cell);

    /// should the cell be an active spreader in the next iteration?
    bool shouldSpread(BiteCell *cell);

protected:
    QStringList allowedProperties();
private:
    DynamicExpression mSpreadFilter;
    DynamicExpression mVoltinism;
    DynamicExpression mSpreadInterval;
    int mSpreadDelay;
    bool mDieAfterDispersal;

    Events mEvents;

};

} // end namespace
#endif // BITELIFECYCLE_H
