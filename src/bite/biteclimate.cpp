#include "bite_global.h"

#include "biteclimate.h"
#include "climate.h"
#include "resourceunit.h"
namespace BITE {

QStringList BiteClimate::mClimateVars = QStringList() << "MAT" << "MAP" << "GDD"
                                                      << "TMonth1" << "TMonth2" << "TMonth3"
                                                      << "TMonth4" << "TMonth5" << "TMonth6"
                                                      << "TMonth7" << "TMonth8" << "TMonth9"
                                                      << "TMonth10" << "TMonth11" << "TMonth12";

BiteClimate::BiteClimate()
{

}

void BiteClimate::setup(QJSValue clim_vars, BiteWrapperCore &wrapper)
{
    qCDebug(biteSetup) << "Setup of climate variables";
    QJSValueIterator it(clim_vars);
    while (it.hasNext()) {
        it.next();

        if (it.name() != "length") {
            QString var_name = it.value().toString();
            int var_index = mClimateVars.indexOf(var_name);
            if (var_index==-1)
                throw IException(QString("The climate variable '%1' is not valid!").arg(var_name));
            qCDebug(biteSetup) << "registering:" << var_name << "with index:" << var_index;

            wrapper.registerClimateVar(var_index, var_name);
        }
    }
}

double BiteClimate::value(int var_index, const ResourceUnit *ru) const
{
    Q_ASSERT(ru != nullptr);
    const Climate *c = ru->climate();
    switch (var_index) {
    case 0: return c->meanAnnualTemperature();  // mean annual temp
    case 1: return c->annualPrecipitation(); // MAP
    case 2: return calculateGDD(c, 5.); // GDD(with base temp 5 degrees)
    }
    if (var_index<3+12) {
        return c->temperatureMonth()[var_index - 3];
    }
    return 0.;
}

double BiteClimate::calculateGDD(const Climate *clim, double threshold_temp) const
{
    double gdd=0.;
    for (const ClimateDay *d = clim->begin(); d!=clim->end(); ++d)
        gdd += qMax(d->mean_temp()-threshold_temp, 0.);
    return gdd;
}


} // namespace
