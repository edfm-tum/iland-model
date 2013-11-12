#ifndef LANDSCAPEOUT_H
#define LANDSCAPEOUT_H

#include "output.h"
#include "expression.h"
#include "standstatistics.h"
#include <QMap>

/** LandscapeOut is aggregated output for the total landscape per species. All values are per hectare values. */
class LandscapeOut : public Output
{
public:
    LandscapeOut();
    virtual void exec();
    virtual void setup();
private:
    Expression mCondition;
    QMap<QString,StandStatistics> mLandscapeStats;
};

#endif // LANDSCAPEOUT_H
