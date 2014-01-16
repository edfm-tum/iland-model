#include "fome_global.h"
#include "fmstp.h"
#include "fmstand.h"
FMSTP::FMSTP()
{
}

// read the setting from the setup-javascript object
void FMSTP::setup()
{

}

// run the management for the forest stand 'stand'
bool FMSTP::execute(FMStand &stand)
{
    switch (stand.phase()) {
        case Regeneration:
    }
}
