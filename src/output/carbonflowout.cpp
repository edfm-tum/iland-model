#include "carbonflowout.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "production3pg.h"

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
              << OutputColumn("GPP_pot", "potential gross primary production, kg C/ru; GPP as calculated ((primary production|here)), " \
                              "sans the effect of the aging modifier f_age; note that a rough estimate of ((sapling growth and competition|#sapling C and N dynamics|sapling GPP)) " \
                              "is added to the GPP of adult trees here.", OutDouble)
              << OutputColumn("GPP_act", "actually relaized gross primary production, kg C/ru; ((primary production|GPP)) including " \
                              "the effect of decreasing productivity with age; note that a rough estimate of "\
                              "((sapling growth and competition|#sapling C and N dynamics|sapling GPP)) is added to the GPP of adult trees here.", OutDouble)
              << OutputColumn("NPP", "net primary production, kg C/ru; calculated as NPP=GPP-Ra; Ra, the autotrophic respiration (kg C/ru) is calculated as"\
                              " a fixed fraction of GPP in iLand (see ((primary production|here)) for details). ", OutDouble)
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

        double gpp_pot = 0.;
        double npp = 0.;
        // calculate the GPP based on the 3PG GPP for the resource units;
        // the NPP is calculated as the sum of NPP of tree individuals
        // an estimate for the saplings layer is added for both pools (based on average dbh and stem number)
        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            gpp_pot += rus->prod3PG().GPPperArea() * ru->stockableArea() * biomassCFraction; // GPP kg Biomass/m2 -> kg/RU -> kg C/RU
            gpp_pot += rus->sapling().carbonGain().C / cAutotrophicRespiration; // add GPP of the saplings (estimate GPP from NPP)
            npp += rus->sapling().carbonGain().C;
        }
        npp += ru->statistics().npp();

        *this << gpp_pot // GPP_pot
        << npp / cAutotrophicRespiration // GPP_act
        << npp // NPP
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

