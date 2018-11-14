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
    void runCell(BiteCell *cell, ABE::FMTreeList *treelist);

protected:
    QStringList allowedProperties();
private:
    DynamicExpression mImpactFilter;
    QString mHostTreeFilter;
    bool mKillAllHostTrees;
    Events mEvents;

};


} // end namespace

#endif // BITEIMPACT_H
