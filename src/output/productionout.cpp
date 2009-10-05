#include "productionout.h"
#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "speciesresponse.h"

ProductionOut::ProductionOut()
{
    setName("Production per month, species and resource unit", "production_month");
    setDescription("Details about the 3PG production submodule on monthly basis and for each species and resource unit.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species()
              << OutputColumn("month", "month of year", OutInteger)
              << OutputColumn("vpdResponse", "respose value VPD", OutDouble)
              << OutputColumn("tempResponse", "respose value temperature", OutDouble)
              << OutputColumn("waterResponse", "respose value soil water", OutDouble)
              << OutputColumn("nitrogenResponse", "respose value nitrogen", OutDouble)
              << OutputColumn("co2Response", "respose value co2", OutDouble)
              << OutputColumn("radiation_m2", "utilizable (!) PAR per m2", OutDouble)
              << OutputColumn("GPP_kg_MJ", "GPP (without Aging) in kg Biomass/m2", OutDouble);
              //<< OutputColumn("GPP_kg_MJ", "GPP (without Aging) in kg Biomass/m2", OutDouble)
              //<< OutputColumn("GPP_kg_MJ", "GPP (without Aging) in kg Biomass/m2", OutDouble);

 }

void ProductionOut::setup()
{
}

void ProductionOut::execute(const ResourceUnitSpecies *rus)
{
    const Production3PG &prod = rus->prod3PG();
    const SpeciesResponse *resp = prod.mResponse;
    for (int i=0;i<12;i++) {
        *this << currentYear() << rus->ru()->index() << rus->species()->id();
        *this << (i+1); // month
        // responses
        *this << resp->vpdResponse()[i] << resp->tempResponse()[i]
              << resp->soilWaterResponse()[i] << resp->nitrogenResponse()
              << resp->co2Response() << prod.mUPAR[i]
              << prod.mGPP[i];
        writeRow();
    }
}

void ProductionOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    foreach(ResourceUnit *ru, m->ruList()) {
        foreach(const ResourceUnitSpecies &rus, ru->ruSpecies()) {
            execute(&rus);
        }
    }
}
