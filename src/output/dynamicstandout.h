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
    Expression mRUFilter;
    struct SDynamicField {
        int agg_index;
        int var_index;
        QString expression;
    };
    QList<SDynamicField> mFieldList;
};

#endif // DYNAMICSTANDOUT_H
