#include "carbonflowout.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"

CarbonFlowOut::CarbonFlowOut()
{

    setName("Carbon fluxes per RU/yr", "carbonflow");
    setDescription("Carbon and nitrogen pools (C and N) per resource unit / year. "\
                   "In the output are aggregated above ground pools (kg/ru) " \
                   "together with below ground pools (kg/ha). \n " \
                   "The area column contains the stockable area and can be used to scale to per unit area values. \n " \
                   "__Note__: the figures for soil pools are per hectare even if the stockable area is below one hectare (scaled to 1ha internally) " \
                   " ");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("area", "total stockable area of the resource unit (m2)", OutInteger)
              << OutputColumn("GPP", "Stem carbon kg/ru", OutDouble)
              << OutputColumn("GPP", "Stem nitrogen kg/ru", OutDouble)
              << OutputColumn("Ra", "branches carbon kg/ru", OutDouble)
              << OutputColumn("Rh", "branches nitrogen kg/ru", OutDouble)
              << OutputColumn("Disturbances", "Foliage carbon kg/ru", OutDouble)
              << OutputColumn("NEP", "Foliage nitrogen kg/ru", OutDouble);
}

void CarbonFlowOut::setup()
{
}


void CarbonFlowOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area
        *this << currentYear() << ru->index() << ru->id() << ru->stockableArea(); // keys

        *this << 0 // GPP
        << 0 // NPP
        << 0 // ra
        << 0 // rh
        << 0 // disturbance
        << 0; // nep
//        // biomass from trees
//        const StandStatistics &s = ru->statistics();
//        *this << s.cStem() << s.nStem()   // stem
//                           << s.cBranch() << s.nBranch()   // branch
//                           << s.cFoliage() << s.nFoliage()   // foliage
//                           << s.cCoarseRoot() << s.nCoarseRoot()   // coarse roots
//                           << s.cFineRoot() << s.nFineRoot();   // fine roots

//        // biomass from regeneration
//        *this << s.cRegeneration() << s.nRegeneration();

//        // biomass from standing dead wood
//        *this << ru->snag()->totalSWD().C << ru->snag()->totalSWD().N   // snags
//                                          << ru->snag()->totalOtherWood().C << ru->snag()->totalOtherWood().N;   // snags, other (branch + coarse root)

//        // biomass from soil (convert from t/ha -> kg/ha)
//        *this << ru->soil()->youngRefractory().C*1000. << ru->soil()->youngRefractory().N*1000.   // wood
//                                                       << ru->soil()->youngLabile().C*1000. << ru->soil()->youngLabile().N*1000.   // litter
//                                                       << ru->soil()->oldOrganicMatter().C*1000. << ru->soil()->oldOrganicMatter().N*1000.;   // soil

        writeRow();
    }

}

