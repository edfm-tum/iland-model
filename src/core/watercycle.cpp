#include "global.h"
#include "watercycle.h"
#include "climate.h"
#include "resourceunit.h"
#include "species.h"
#include "model.h"

WaterCycle::WaterCycle()
{
    mSoilDepth = 0;
}

void WaterCycle::setup(const ResourceUnit *ru)
{
    mRU = ru;
    // get values...
    mFieldCapacity = 0.; // on top
    const XmlHelper &xml=GlobalSettings::instance()->settings();
    mSoilDepth = xml.valueDouble("model.site.soilDepth", 0.) * 10; // convert from cm to mm
    mCanopy.setup();
    mPsi_koeff_b = -3;
    mPsi_ref = -0.35; // kPa
    mRho_ref = 0.42;
    mPermanentWiltingPoint = heightFromPsi(-1500);
    mFieldCapacity = heightFromPsi(-15);
    mContent = mFieldCapacity; // start with full water content (in the middle of winter)
}

/** functions to calculate
*/
inline double WaterCycle::psiFromHeight(const double mm) const
{
    // psi_x = psi_ref * ( rho_x / rho_ref) ^ b
    if (mm<0.001)
        return -100000000;
    double psi_x = mPsi_ref * pow((mm / mSoilDepth / mRho_ref),mPsi_koeff_b);
    return psi_x;
}

inline double WaterCycle::heightFromPsi(const double psi_kpa) const
{
    // rho_x = rho_ref * (psi_x / psi_ref)^(1/b)
    double h = mSoilDepth * mRho_ref * pow(psi_kpa / mPsi_ref, 1./mPsi_koeff_b);
    return h;
}

void WaterCycle::getStandValues()
{
    mLAINeedle=mLAIBroadleaved=0.;
    mCanopyConductance=0.;
    double lai;
    double psi_max = 0.;
    foreach(const ResourceUnitSpecies &rus, mRU->ruSpecies()) {
        lai = rus.constStatistics().leafAreaIndex();
        if (rus.species()->isConiferous())
            mLAINeedle+=lai;
        else
            mLAIBroadleaved+=lai;
        mCanopyConductance += rus.species()->canopyConductance() * lai; // weigh with LAI
        psi_max += rus.species()->psiMax() * lai; // weigh with LAI
    }
    double total_lai = mLAIBroadleaved+mLAINeedle;
    if (total_lai>0.) {
        mCanopyConductance /= total_lai;
        mPermanentWiltingPoint = heightFromPsi(1000. * psi_max / total_lai); // (psi/total_lai) = weighted average Psi (MPa) -> convert to kPa
    } else {
        mCanopyConductance = 0.02;
        mPermanentWiltingPoint = heightFromPsi(-1500);// -1.5MPa
    }
    if (total_lai < 3.) {
        // following Landsberg and Waring: when LAI is < 3, a linear "ramp" from 0 to 3 is assumed
        // http://iland.boku.ac.at/water+cycle#transpiration_and_canopy_conductance
        mCanopyConductance *= total_lai / 3.;
    }
    qDebug() << "WaterCycle:getStandValues: LAI needle" << mLAINeedle << "LAI Broadl:"<< mLAIBroadleaved << "weighted avg. Conductance (m/2):" << mCanopyConductance;
}

/// Main Water Cycle function. This function triggers all water related tasks for
/// one simulation year.
/// @sa http://iland.boku.ac.at/water+cycle
void WaterCycle::run()
{
    // preparations (once a year)
    getStandValues(); // fetch canopy characteristics from iLand
    mCanopy.setStandParameters(mLAINeedle,
                               mLAIBroadleaved,
                               mCanopyConductance);


    // main loop over all days of the year
    double prec_mm, prec_after_interception, prec_to_soil, et, excess;
    const ClimateDay *day = mRU->climate()->begin();
    const ClimateDay *end = mRU->climate()->end();
    int doy=0;
    double total_excess = 0.;
    for (; day<end; ++day, ++doy) {
        // (1) precipitation of the day
        prec_mm = day->preciptitation;
        // (2) interception by the crown
        prec_after_interception = mCanopy.flow(prec_mm, day->temperature);
        // (3) storage in the snow pack
        prec_to_soil = mSnowPack.flow(prec_after_interception, day->temperature);
        // (4) add rest to soil
        mContent += prec_to_soil;

        excess = 0.;
        if (mContent>mFieldCapacity) {
            // excess water runoff
            excess = mContent - mFieldCapacity;
            total_excess += excess;
            mContent = mFieldCapacity;
        }

        // calculate the relative water content
        mRelativeContent[doy] = currentRelContent();
        mPsi[doy] = psiFromHeight(mContent);
        // (5) transpiration of the vegetation
        et = mCanopy.evapotranspiration3PG(day, mRU->climate()->daylength_h(doy));

        mContent -= et; // reduce content (transpiration)
        mContent += mCanopy.interception(); // add intercepted water that is *not* evaporated to the soil again
        
        if (mContent<mPermanentWiltingPoint) {
            et -= mPermanentWiltingPoint - mContent; // reduce et (for bookkeeping)
            mContent = mPermanentWiltingPoint;
        }

        //DBGMODE(
            if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dWaterCycle)) {
                DebugList &out = GlobalSettings::instance()->debugList(day->id(), GlobalSettings::dWaterCycle);
                // climatic variables
                out << day->id() << day->temperature << day->vpd << day->preciptitation << day->radiation;
                // fluxes
                out << prec_after_interception << prec_to_soil << et << mCanopy.evaporationCanopy()
                        << mRelativeContent[doy]*mSoilDepth << mPsi[doy] << excess;
                // other states
                out << mSnowPack.snowPack();

            }
        //); // DBGMODE()

    }
}


namespace Water {

/** calculates the input/output of water that is stored in the snow pack.
  The approach is similar to Picus 1.3 and ForestBGC (Running, 1988).
  Returns the amount of water that exits the snowpack (precipitation, snow melt) */
double SnowPack::flow(const double &preciptitation_mm, const double &temperature)
{
    if (temperature>0.) {
        if (mSnowPack==0.)
            return preciptitation_mm; // no change
        else {
            // snow melts
            const double melting_coefficient = 0.7; // mm/°C
            double melt = qMin(temperature * melting_coefficient, mSnowPack);
            mSnowPack -=melt;
            return preciptitation_mm + melt;
        }
    } else {
        // snow:
        mSnowPack += preciptitation_mm;
        return 0.; // no output.
    }

}


/** Interception in crown canopy.
    Calculates the amount of preciptitation that does not reach the ground and
    is stored in the canopy. The approach is adopted from Picus 1.3.
    Returns the amount of precipitation (mm) that surpasses the canopy layer.
    @sa http://iland.boku.ac.at/water+cycle#precipitation_and_interception */
double Canopy::flow(const double &preciptitation_mm, const double &temperature)
{
    // sanity checks
    mInterception = 0.;
    mEvaporation = 0.;
    if (!mLAI)
        return preciptitation_mm;
    if (!preciptitation_mm)
        return 0.;
    double max_interception_mm=0.; // maximum interception based on the current foliage
    double max_storage_mm=0.; // maximum storage in canopy

    if (mLAINeedle>0.) {
        // (1) calculate maximum fraction of thru-flow the crown (based on precipitation)
        double max_flow_needle = 0.9 * sqrt(1.03 - exp(-0.055*preciptitation_mm));
        max_interception_mm += preciptitation_mm *  (1. - max_flow_needle * mLAINeedle/mLAI);
        // (2) calculate maximum storage potential based on the current LAI
        double max_storage_needle = 4. * (1. - exp(-0.55*mLAINeedle) );
        max_storage_mm += max_storage_needle;
    }

    if (mLAIBroadleaved>0.) {
        // (1) calculate maximum fraction of thru-flow the crown (based on precipitation)
        double max_flow_broad = 0.9 * pow(1.22 - exp(-0.055*preciptitation_mm), 0.35);
        max_interception_mm += preciptitation_mm *  (1. - max_flow_broad * mLAIBroadleaved/mLAI);
        // (2) calculate maximum storage potential based on the current LAI
        double max_storage_broad = 2. * (1. - exp(-0.5*mLAIBroadleaved) );
        max_storage_mm += max_storage_broad;
    }

    // (3) calculate actual interception and store for evaporation calculation
    mInterception = qMin( max_storage_mm, max_interception_mm );

    // (4) reduce preciptitaion by the amount that is intercepted by the canopy
    DBG_IF(preciptitation_mm < mInterception,"water_cycle", "prec < interception");
    return preciptitation_mm - mInterception;

}

/// sets up the canopy. fetch some global parameter values...
void Canopy::setup()
{
    mHeatCapacityAir = Model::settings().heatCapacityAir; // J/kg/°C
    mAirDensity = Model::settings().airDensity; // kg / m3
    double airPressure = Model::settings().airPressure; // mbar
    double heat_capacity_kJ = mHeatCapacityAir / 1000; // convert to: kJ/kg/°C
    const double latent_heat_water = 2450;// kJ/kg
    // calc psychrometric constant (in mbar/°C): see also http://en.wikipedia.org/wiki/Psychrometric_constant
    mPsychrometricConstant = heat_capacity_kJ*airPressure/latent_heat_water/0.622;
}

void Canopy::setStandParameters(const double LAIneedle, const double LAIbroadleave, const double maxCanopyConductance)
{
    mLAINeedle = LAIneedle;
    mLAIBroadleaved=LAIbroadleave;
    mLAI=LAIneedle+LAIbroadleave;
    mAvgMaxCanopyConductance = maxCanopyConductance;
}

/** calculate the daily evaporation/transpiration using the Penman-Monteith-Equation.
   The application of the equation follows broadly Running (1988).
   Returns the total sum of evaporation+transpiration in mm of the day. */
double Canopy::evapotranspirationBGC(const ClimateDay *climate, const double daylength_h)
{
    double vpd_mbar = climate->vpd * 10.; // convert from kPa to mbar
    double temperature = climate->temperature; // average temperature of the day (°C)
    double daylength = daylength_h * 3600.; // daylength in seconds (convert from length in hours)
    double rad = climate->radiation / daylength * 1000000; //convert from MJ/m2 (day sum) to average radiation flow W/m2 [MJ=MWs -> /s * 1,000,000
    const double aerodynamic_resistance = 5.; // m/s: aerodynamic resistance of the canopy is considered being constant
    const double latent_heat = 2257000.; // Latent heat of vaporization. Energy required per unit mass of water vaporized [J kg-1]

    // (1) calculate some intermediaries
    // current canopy conductance: is calculated following Landsberg & Waring
    // note: here we use vpd again in kPa.
    double current_canopy_conductance;
    current_canopy_conductance = mAvgMaxCanopyConductance * exp(-2.5 * climate->vpd);

    // saturation vapor pressure (Running 1988, Eq. 1) in mbar
    double svp = 6.1078 * exp((17.269 * temperature) / (237.3 + temperature) );
    // the slope of svp is, thanks to http://www.wolframalpha.com/input/?i=derive+y%3D6.1078+exp+((17.269x)/(237.3%2Bx))
    double svp_slope = svp * ( 17.269/(237.3+temperature) - 17.269*temperature/((237.3+temperature)*(237.3+temperature)) );

    double et; // transpiration in mm (follows Eq.(8) of Running, 1988).
    // note: RC (resistance of canopy) = 1/CC (conductance of canopy)
    double dim = svp_slope + mPsychrometricConstant*(1. + 1. / (current_canopy_conductance*aerodynamic_resistance));
    double dayl = daylength*mLAI / latent_heat;
    double upper = svp_slope*rad + mHeatCapacityAir*mAirDensity * vpd_mbar/aerodynamic_resistance;
    et = upper / dim * dayl;

    // now calculate the evaporation from intercepted preciptitaion in the canopy: 1+rc/ra -> 1
    if (mInterception>0.) {
        double dim_evap = svp_slope + mPsychrometricConstant;
        double pot_evap = upper / dim_evap * dayl;
        double evap = qMin(pot_evap, mInterception);
        mEvaporation = evap;
    }
    return et;
}


/** calculate the daily evaporation/transpiration using the Penman-Monteith-Equation.
   This version is based on 3PG. See the Visual Basic Code in 3PGjs.xls.
   Returns the total sum of evaporation+transpiration in mm of the day. */
double Canopy::evapotranspiration3PG(const ClimateDay *climate, const double daylength_h)
{
    double vpd_mbar = climate->vpd * 10.; // convert from kPa to mbar
    double temperature = climate->temperature; // average temperature of the day (°C)
    double daylength = daylength_h * 3600.; // daylength in seconds (convert from length in hours)
    double rad = climate->radiation / daylength * 1000000; //convert from MJ/m2 (day sum) to average radiation flow W/m2 [MJ=MWs -> /s * 1,000,000

    //: Landsberg original: const double e20 = 2.2;  //rate of change of saturated VP with T at 20C
    const double VPDconv = 0.000622; //convert VPD to saturation deficit = 18/29/1000
    const double latent_heat = 2460000.; // Latent heat of vaporization. Energy required per unit mass of water vaporized [J kg-1]

    double gBL  = 0.2; // boundary layer conductance
    // canopy conductance scales linearly from 0 to LAI=3 and is constant for LAI > 3.
    double gC = mAvgMaxCanopyConductance * exp(-2.5 * climate->vpd);
    if (mLAI<3.)
        gC *= mLAI / 3.;

    double defTerm = mAirDensity * latent_heat * (vpd_mbar * VPDconv) * gBL;
        // saturation vapor pressure (Running 1988, Eq. 1) in mbar
    double svp = 6.1078 * exp((17.269 * temperature) / (237.3 + temperature) );
    // the slope of svp is, thanks to http://www.wolframalpha.com/input/?i=derive+y%3D6.1078+exp+((17.269x)/(237.3%2Bx))
    double svp_slope = svp * ( 17.269/(237.3+temperature) - 17.269*temperature/((237.3+temperature)*(237.3+temperature)) );

    double div = (1. + svp_slope + gBL / gC);
    double Etransp = (svp_slope * rad + defTerm) / div;
    double canopy_transiration = Etransp / latent_heat * daylength;

    if (mInterception>0.) {
        // we assume that for evaporation from leaf surface gBL/gC -> 0
        double div_evap = 1 + svp_slope;
        double evap = (svp_slope*rad + defTerm) / div_evap / latent_heat * daylength;
        evap = qMin(evap, mInterception);
        mEvaporation = evap;
    }
    return canopy_transiration;
}

} // end namespace
