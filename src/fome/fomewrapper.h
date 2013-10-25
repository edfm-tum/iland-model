#ifndef FOMEWRAPPER_H
#define FOMEWRAPPER_H

#include "expressionwrapper.h"
class Activity;
class FMStand;


/** FOMEWrapper provides the context for the Forest Management Engine
 *  This wrapper blends activties, stand variables, and agent variables together.
*/

class FOMEWrapper: public ExpressionWrapper
{
public:
    FOMEWrapper(): mActivity(0)  {}
    FOMEWrapper(const Activity *act, const FMStand *stand): mActivity(act), mStand(stand) {}
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    void buildVarList();
    double valueActivity(const int variableIndex);
    double valueStand(const int variableIndex);
    double valueSite(const int variableIndex);
    const Activity *mActivity;
    const FMStand *mStand;
};


#endif // FOMEWRAPPER_H
