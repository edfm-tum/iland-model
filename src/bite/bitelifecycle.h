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
