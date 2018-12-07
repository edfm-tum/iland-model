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
    void beforeRun();
    void afterSetup();

protected:
    QStringList allowedProperties();

    Events mEvents;
private:
    void calculateLogisticGrowth(BiteCell *cell);
    Grid<double> mHostBiomass;
    Grid<double> mAgentBiomass;
    Grid<double> mImpact;
    QString mHostTreeFilter;
    DynamicExpression mCalcHostBiomass; // calculate host biomass based on trees / cells
    DynamicExpression mMortality;
    Expression mGrowthFunction; // (logistic) growth function
    Expression mGrowthRateFunction; // function to calculate the growth rate 'r'
    int mGrowthIterations;  // number of iterations during a time step (year) for updating agent/host biomass
    double mGrowthConsumption;  // consumption rate: cons= kg host biomass / kg agent per year
    bool mVerbose;

};


} // end namespace
#endif // BITEBIOMASS_H
