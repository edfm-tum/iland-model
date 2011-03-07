#include "carbonflowout.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "production3pg.h"
#include "soil.h"

CarbonFlowOut::CarbonFlowOut()
{

    setName("Carbon fluxes per RU/yr", "carbonflow");
    setDescription("Carbon fluxes per resource unit and year. Note that all fluxes are reported on a per ru basis, " \
                   "i.e. on the actual simulated area. Thus summing over all ru should give the overall C fluxes for"\
                   " the simulated landscape. Fluxes that are internally calculated on a per ha basis thus need to be "\
                   "scaled to the stockable area. Furthermore, the following sign convention is used in iLand: fluxes "\
                   "from the atmosphere to the ecosystem are positive, while C leaving the ecosystem is reported as negative C flux.");
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
              << OutputColumn("Rh", "heterotrophic respiration, kg C/ru; sum of C released to the atmosphere from detrital pools, i.e."\
                              " ((snag dynamics|#Snag decomposition|snags)), ((soil C and N cycling|downed deadwood, litter, and mineral soil)).", OutDouble)
              << OutputColumn("dist_loss", "disturbance losses, kg C/ru; C that leaves the ecosystem as a result of disturbances, e.g. fire consumption", OutDouble)
              << OutputColumn("mgmt_loss", "management losses, kg C/ru; C that leaves the ecosystem as a result of management interventions, e.g. harvesting", OutDouble)
              << OutputColumn("NEP", "net ecosytem productivity kg C/ru, NEP=NPP - Rh - disturbance losses - management losses. "\
                              "Note that NEP is also equal to the total net changes over all ecosystem C pools, as reported in the "\
                              "carbon output (cf. [http://www.jstor.org/stable/3061028|Randerson et al. 2002])", OutDouble);
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
        if (!ru->soil() || !ru->snag()) {
            qDebug() << "CarbonFlowOut::exec: resource unit without soil or snags module - no output generated.";
            continue;
        }
        *this << currentYear() << ru->index() << ru->id() << ru->stockableArea(); // keys

        double gpp_pot = 0.;
        double npp = 0.;
        // calculate the GPP based on the 3PG GPP for the resource units;
        // the NPP is calculated as the sum of NPP of tree individuals
        // an estimate for the saplings layer is added for both pools (based on average dbh and stem number)
        foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
            gpp_pot += rus->prod3PG().GPPperArea() * ru->stockedArea() * biomassCFraction; // GPP kg Biomass/m2 -> kg/RU -> kg C/RU
            gpp_pot += rus->sapling().carbonGain().C / cAutotrophicRespiration; // add GPP of the saplings (estimate GPP from NPP)
            npp += rus->sapling().carbonGain().C;
        }
        npp += ru->statistics().npp();
        double to_atm = ru->snag()->fluxToAtmosphere().C; // from snags, kg/ha
        to_atm += ru->soil()->fluxToAtmosphere().C * ru->stockableArea()/10.; // soil: t/ha -> t/m2 -> kg/ha

        double to_dist = ru->snag()->fluxToDisturbance().C;
        to_dist += ru->soil()->fluxToDisturbance().C * ru->stockableArea()/10.;

        double to_harvest = ru->snag()->fluxToExtern().C;

        double nep = npp - to_atm - to_harvest - to_dist;

        *this << gpp_pot // GPP_pot
        << npp / cAutotrophicRespiration // GPP_act
        << npp // NPP
        << to_atm // rh
        << to_dist // disturbance
        << to_harvest // management loss
        << nep; // nep

        writeRow();
    }

}

