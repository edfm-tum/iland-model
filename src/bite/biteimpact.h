#ifndef BITEIMPACT_H
#define BITEIMPACT_H

#include "biteitem.h"
#include "bitecellscript.h"

namespace BITE {


class BiteImpact: public BiteItem
{
    Q_OBJECT

public:
    Q_INVOKABLE BiteImpact(QJSValue obj);
    void setup(BiteAgent *parent_agent);
    QString info();
    // void notify(BiteCell *cell, BiteCell::ENotification what);

public slots:
    void afterSetup();
    void runCell(BiteCell *cell, ABE::FMTreeList *treelist);

protected:
    QStringList allowedProperties();
private:
    void doImpact(double to_remove, BiteCell *cell, ABE::FMTreeList *treelist);
    enum ImpactTarget {KillAll, Foliage, Invalid};
    enum ImpactMode {Relative, RemoveAll, RemovePart};
    ImpactTarget mImpactTarget;
    ImpactMode mImpactMode;
    DynamicExpression mImpactFilter;
    QString mHostTreeFilter;
    bool mSimulate;
    Events mEvents;
    int iAgentImpact;
    QString mImportOrder;
    bool mVerbose;

};


} // end namespace

#endif // BITEIMPACT_H
