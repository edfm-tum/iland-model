#ifndef FOMEWRAPPER_H
#define FOMEWRAPPER_H

#include "expressionwrapper.h"

namespace AMIE {
class FMStand;


/** FOMEWrapper provides the context for the Forest Management Engine
 *  This wrapper blends activties, stand variables, and agent variables together.
*/

class FOMEWrapper: public ExpressionWrapper
{
public:
    FOMEWrapper(): mStand(0)  {}
    FOMEWrapper(const FMStand *stand):  mStand(stand) {}
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    void buildVarList();
    double valueActivity(const int variableIndex);
    double valueStand(const int variableIndex);
    double valueSite(const int variableIndex);
    const FMStand *mStand;
};

} // namespace

#endif // FOMEWRAPPER_H
