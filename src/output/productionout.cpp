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
              << OutputColumn("minResponse", "Average of daily minimum respones values of vpd, temperature, soilwater", OutDouble)
              << OutputColumn("vpdResponse", "monthly average of daily respose value VPD", OutDouble)
              << OutputColumn("tempResponse", "monthly average of daily respose value temperature", OutDouble)
              << OutputColumn("waterResponse", "monthly average of daily respose value soil water", OutDouble)
              << OutputColumn("nitrogenResponse", "yearly respose value nitrogen", OutDouble)
              << OutputColumn("co2Response", "yearly respose value co2", OutDouble)
              << OutputColumn("radiation_m2", "utilizable (!) PAR in MJ per m2 and month", OutDouble)
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
        *this <<  resp->minimumResponses()[i]
              <<  resp->vpdResponse()[i] << resp->tempResponse()[i]
              << resp->soilWaterResponse()[i] << resp->nitrogenResponse()
              << resp->co2Response()[i] << prod.mUPAR[i]
              << prod.mGPP[i];
        writeRow();
    }
}

void ProductionOut::exec()
{
    DebugTimer t("ProductionOut");
    Model *m = GlobalSettings::instance()->model();

    foreach(ResourceUnit *ru, m->ruList()) {
        foreach(const ResourceUnitSpecies &rus, ru->ruSpecies()) {
            execute(&rus);
        }
    }
}
