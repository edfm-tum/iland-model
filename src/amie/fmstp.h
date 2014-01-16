#ifndef FOMESTP_H
#define FOMESTP_H

#include "fmstand.h"

/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */

class FMSTP
{
public:
    FMSTP();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    /// read the optionfile
    void setup();

    /// main function that runs the current program for stand 'stand'
    bool execute(FMStand &stand);
private:

};

#endif // FOMESTP_H
