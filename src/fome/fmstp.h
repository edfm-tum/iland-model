#ifndef FOMESTP_H
#define FOMESTP_H

#include "activity.h"

/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */

class FMSTP
{
public:
    FMSTP();
private:
    Activity *mTendingActivity;
    Activity *mThinningActivity;
    Activity *mRegenerationActivity;

};

#endif // FOMESTP_H
