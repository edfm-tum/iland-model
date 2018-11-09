#ifndef BITEBIOMASS_H
#define BITEBIOMASS_H

#include "biteitem.h"
#include "bitecellscript.h"
#include "grid.h"

namespace BITE {

class BiteBiomass: public BiteItem
{
    Q_OBJECT

public:
    Q_INVOKABLE BiteBiomass(QJSValue obj);
    void setup(BiteAgent *parent_agent);
    QString info();
    void notify(BiteCell *cell, BiteCell::ENotification what);


public slots:
    void runCell(BiteCell *cell, ABE::FMTreeList *treelist);

protected:
    QStringList allowedProperties();

    Events mEvents;
private:
    Grid<double> mCarryingCapacity;
    Grid<double> mAgentBiomass;
    QString mHostTreeFilter;
    DynamicExpression mCalcCCTree;
    DynamicExpression mCalcCCCell;
    DynamicExpression mMortality;

};


} // end namespace
#endif // BITEBIOMASS_H
