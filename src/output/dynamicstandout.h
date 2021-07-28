/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland-model.org
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

#ifndef DYNAMICSTANDOUT_H
#define DYNAMICSTANDOUT_H

#include "output.h"
#include "expression.h"

class DynamicStandOut : public Output
{
public:
    DynamicStandOut();
    virtual void exec();
    virtual void setup();
private:
    void extractByResourceUnit(const bool by_species);
    Expression mRUFilter;
    Expression mTreeFilter;
    Expression mCondition;
    Expression mConditionRU;
    struct SDynamicField {
        int agg_index;
        int var_index;
        QString expression;
    };
    QList<SDynamicField> mFieldList;
};

#endif // DYNAMICSTANDOUT_H
