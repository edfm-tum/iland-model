#ifndef BITECLIMATE_H
#define BITECLIMATE_H

#include <QJSValue>

#include "bitewrapper.h"
class ResourceUnit; // forward
namespace BITE {


class BiteClimate
{
public:
    BiteClimate();
    void setup(QJSValue clim_vars, BITE::BiteWrapperCore &wrapper);

    /// retrieve the climate variable with the given index (raise an exception if invalid)
    double value(int var_index, const ResourceUnit *ru) const;
private:
    static QStringList mClimateVars;

};

} // namespace
#endif // BITECLIMATE_H
