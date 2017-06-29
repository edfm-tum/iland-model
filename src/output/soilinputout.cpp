#include "soilinputout.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "soil.h"

SoilInputOut::SoilInputOut()
{
    setName("Carbon input to the soil per RU/yr", "soilinput");
    setDescription("The output contains all carbon inputs to the soil layer (per RU) and the climate modifier re. The data of the output " \
                   "can be used for estimating soil carbon parameters. ");


    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("input_lab", "input for labile carbon (i.e. fine debris) in t/ha (scaled to full ha)", OutDouble)
              << OutputColumn("input_ref", "input for refractory carbon (i.e. coarse woody debris) in t/ha (scaled to full ha)", OutDouble)
              << OutputColumn("re", "climate factor 're' for snag/carbon decay", OutDouble);

}

void SoilInputOut::exec()
{
    Model *m = GlobalSettings::instance()->model();


    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area
        if (!ru->soil()) {
            qDebug() << "SoilInputOut::exec: resource unit without soil  module - no output generated.";
            continue;
        }

        *this << currentYear() << ru->index() << ru->id();
        *this << ru->soil()->mInputLab.C << ru->soil()->mInputRef.C << ru->soil()->mRE;
        writeRow();
    }
}

void SoilInputOut::setup()
{

}
