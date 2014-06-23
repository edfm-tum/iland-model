#ifndef ACTTHINNING_H
#define ACTTHINNING_H
#include "activity.h"

namespace ABE {

class FMSTP; // forward
class FMStand; // forward


class ActThinning : public Activity
{
public:
    ActThinning(FMSTP *parent);
    enum ThinningType { Invalid, FromBelow, FromAbove, Custom, Selection};
    QString type() const;
    void setup(QJSValue value);
    bool evaluate(FMStand *stand);
    bool execute(FMStand *stand);
private:
    struct SCustomThinning {
        bool usePercentiles; ///< if true, classes relate to percentiles, if 'false' classes relate to relative dbh classes
        bool removal; ///< if true, classes define removals, if false trees should *stay* in the class
        bool relative; ///< if true, values are per cents, if false, values are absolute values per hectare
        QString targetVariable; ///< target variable ('volume', 'basalArea', 'stems') / ha
        QVector<double> classValues; ///< class values (the number of values defines the number of classes)
        QVector<int> classPercentiles; ///< percentiles [0..100] for the classes (count = count(classValues) + 1
        double minDbh; ///< only trees with dbh > minDbh are considered (default: 0)
        int remainingStems; ///< minimum remaining stems/ha (>minDbh)
    };
    SCustomThinning mCustom;
    void setupCustom(QJSValue value);
    bool evaluateCustom(FMStand *stand);
    ThinningType mThinningType;

};


} // namespace
#endif // ACTTHINNING_H
