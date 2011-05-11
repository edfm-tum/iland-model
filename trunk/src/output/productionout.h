#ifndef PRODUCTIONOUT_H
#define PRODUCTIONOUT_H
#include "output.h"
#include "expression.h"
class ResourceUnitSpecies;
/** ProductionOut describes finegrained production details on the level of resourceunits per month. */
class ProductionOut : public Output
{
public:
    ProductionOut();
    void execute(const ResourceUnitSpecies *rus);
    virtual void exec();
    virtual void setup();
};

#endif // PRODUCTIONOUT_H
