/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

#ifndef TEST_CORE_ENTITIES_H
#define TEST_CORE_ENTITIES_H

#include <QObject>

class TestCoreEntities : public QObject
{
    Q_OBJECT
private slots:
    void saplingCellStateAndSlots();
    void cnPairPoolAndSnagThresholds();
    void soilValidationMassBalanceAndDisturbance();
    void timeEventsXmlAndJsExecution();
    void abeUnitObjAndSchedulerClearStand();
};

#endif // TEST_CORE_ENTITIES_H
