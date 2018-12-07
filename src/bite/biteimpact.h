/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
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
    double doImpact(double to_remove, BiteCell *cell, ABE::FMTreeList *treelist);
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
