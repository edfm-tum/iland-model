#ifndef BITECOLONIZATION_H
#define BITECOLONIZATION_H

#include "biteitem.h"
#include "bitecellscript.h"

namespace BITE {

class BiteCell; // forward

class BiteColonization: BiteItem
{
    Q_OBJECT
    //Q_PROPERTY(ScriptGrid* grid READ grid)
public:

    BiteColonization();
    Q_INVOKABLE BiteColonization(QJSValue obj);
    void setup(BiteAgent *parent_agent);
    void afterSetup();

    void runCell(BiteCell *cell, ABE::FMTreeList *treelist);
protected:
    QStringList allowedProperties();

private:
    Constraints mCellConstraints;
    Constraints mTreeConstraints;
    DynamicExpression mDispersalFilter;
    Events mEvents;
    DynamicExpression mInitialAgentBiomass;
    int iAgentBiomass;

};

} // end namespace
#endif // BITECOLONIZATION_H
