#include "amie_global.h"
#include "global.h"

#include "fmunit.h"
namespace AMIE {

void evaluateActivities()
{
    // loop over all stands
    // and evaluate for each stand all activites
}

void FMUnit::setId(const QString &id)
{
    mId = id;
    mIndex = mId.toInt();
}

} // namesapce
