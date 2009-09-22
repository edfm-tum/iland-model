#include "standout.h"
#include "helper.h"
#include "model.h"
#include "ressourceunit.h"
#include "species.h"


StandOut::StandOut()
{
    setName("Stand by species/RU", "stand");
    setDescription("Output of aggregates on the level of RU x species.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species()
              << OutputColumn("v1", "a double value", OutDouble);
 }

void StandOut::setup()
{
}

void StandOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    foreach(RessourceUnit *ru, m->ruList()) {
        foreach(const RessourceUnitSpecies &rus, ru->ruSpecies()) {
            *this << currentYear() << ru->index() << rus.species()->id(); // keys
            writeRow();
        }
    }
}
