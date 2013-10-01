/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "global.h"
#include "watercycle.h"
#include "climate.h"
#include "resourceunit.h"
#include "species.h"
#include "model.h"
#include "helper.h"
#include "modules.h"

/** @class WaterCycle
  @ingroup core
  simulates the water cycle on a ResourceUnit.
  The WaterCycle is simulated with a daily time step on the spatial level of a ResourceUnit. Related are
  the snow module (SnowPack), and Canopy module that simulates the interception (and evaporation) of precipitation and the
  transpiration from the canopy.
  The WaterCycle covers the "soil water bucket". Main entry function is run().

  See http://iland.boku.ac.at/water+cycle
  */

WaterCycle::WaterCycle()
{
    mSoilDepth = 0;
    mLastYear = -1;
}

void WaterCycle::setup(const ResourceUnit *ru)
{
    mRU = ru;
    // get values...
    mFieldCapacity = 0.; // on top
    const XmlHelper &xml=GlobalSettings::instance()->settings();
    mSoilDepth = xml.valueDouble("model.site.soilDepth", 0.) * 10; // convert from cm to mm
    double pct_sand = xml.valueDouble("model.site.pctSand");
    double pct_silt = xml.valueDouble("model.site.pctSilt");
    double pct_clay = xml.valueDouble("model.site.pctClay");
    if (fabs(100. - (pct_sand + pct_silt + pct_clay)) > 0.01)
        throw IException(QString("Setup Watercycle: soil composition percentages do not sum up to 100. Sand: %1, Silt: %2 Clay: %3").arg(pct_sand).arg(pct_silt).arg(pct_clay));

    // calculate soil characteristics based on empirical functions (Schwalm & Ek, 2004)
    // note: the variables are percentages [0..100]
    mPsi_sat = -exp((1.54 - 0.0095*pct_sand + 0.0063*pct_silt) * log(10)) * 0.000098; // Eq. 83
    mPsi_koeff_b = -( 3.1 + 0.157*pct_clay - 0.003*pct_sand );  // Eq. 84
    mTheta_sat = 0.01 * (50.5 - 0.142*pct_sand - 0.037*pct_clay); // Eq. 78
    mCanopy.setup();

    mPermanentWiltingPoint = heightFromPsi(-4000); // maximum psi is set to a constant of -4MPa
    if (xml.valueBool("model.settings.waterUseSoilSaturation",false)==false) {
        mFieldCapacity = heightFromPsi(-15);
    } else {
        // =-EXP((1.54-0.0095* pctSand +0.0063* pctSilt)*LN(10))*0.000098
        double psi_sat = -exp((1.54-0.0095 * pct_sand + 0.0063*pct_silt)*log(10.))*0.000098;
        mFieldCapacity = heightFromPsi(psi_sat);
        if (logLevelDebug()) qDebug() << "psi: saturation " << psi_sat << "field capacity:" << mFieldCapacity;
    }

    mContent = mFieldCapacity; // start with full water content (in the middle of winter)
    if (logLevelDebug()) qDebug() << "setup of water: Psi_sat (kPa)" << mPsi_sat << "Theta_sat" << mTheta_sat << "coeff. b" << mPsi_koeff_b;
    mCanopyConductance = 0.;
    mLastYear = -1;

    // canopy settings
    mCanopy.mNeedleFactor = xml.valueDouble("model.settings.interceptionStorageNeedle", 4.);
    mCanopy.mDecidousFactor = xml.valueDouble("model.settings.interceptionStorageBroadleaf", 2.);
    mSnowPack.mSnowTemperature = xml.valueDouble("model.settings.snowMeltTemperature", 0.);
}

/** function to calculate the water pressure [saugspannung] for a given amount of water.
    returns water potential in kPa.
  see http://iland.boku.ac.at/water+cycle#transpiration_and_canopy_conductance */
inline double WaterCycle::psiFromHeight(const double mm) const
{
    // psi_x = psi_ref * ( rho_x / rho_ref) ^ b
    if (mm<0.001)
        return -100000000;
    double psi_x = mPsi_sat * pow((mm / mSoilDepth / mTheta_sat),mPsi_koeff_b);
    return psi_x; // pis
}

/// calculate the height of the water column for a given pressure
/// return water amount in mm
/// see http://iland.boku.ac.at/water+cycle#transpiration_and_canopy_conductance
inline double WaterCycle::heightFromPsi(const double psi_kpa) const
{
    // rho_x = rho_ref * (psi_x / psi_ref)^(1/b)
    double h = mSoilDepth * mTheta_sat * pow(psi_kpa / mPsi_sat, 1./mPsi_koeff_b);
    return h;
}

/// get canopy characteristics of the resource unit.
/// It is important, that species-statistics are valid when this function is called (LAI)!
void WaterCycle::getStandValues()
{
    mLAINeedle=mLAIBroadleaved=0.;
    mCanopyConductance=0.;
    const double ground_vegetationCC = 0.02;
    double lai;
    foreach(const ResourceUnitSpecies *rus, mRU->ruSpecies()) {
        lai = rus->constStatistics().leafAreaIndex();
        if (rus->species()->isConiferous())
            mLAINeedle+=lai;
        else
            mLAIBroadleaved+=lai;
        mCanopyConductance += rus->species()->canopyConductance() * lai; // weigh with LAI
    }
    double total_lai = mLAIBroadleaved+mLAINeedle;

    // handle cases with LAI < 1 (use generic "ground cover characteristics" instead)
    /* The LAI used here is derived from the "stockable" area (and not the stocked area).
       If the stand has gaps, the available trees are "thinned" across the whole area. Otherwise (when stocked area is used)
       the LAI would overestimate the transpiring canopy. However, the current solution overestimates e.g. the interception.
       If the "thinned out" LAI is below one, the rest (i.e. the gaps) are thought to be covered by ground vegetation.
    */
    if (total_lai<1.) {
        mCanopyConductance+=(ground_vegetationCC)*(1. - total_lai);
        total_lai = 1.;
    }
    mCanopyConductance /= total_lai;

    if (total_lai < Model::settings().laiThresholdForClosedStands) {
        // following Landsberg and Waring: when LAI is < 3 (default for laiThresholdForClosedStands), a linear "ramp" from 0 to 3 is assumed
        // http://iland.boku.ac.at/water+cycle#transpiration_and_canopy_conductance
        mCanopyConductance *= total_lai / Model::settings().laiThresholdForClosedStands;
    }
    if (logLevelInfo()) qDebug() << "WaterCycle:getStandValues: LAI needle" << mLAINeedle << "LAI Broadl:"<< mLAIBroadleaved << "weighted avg. Conductance (m/2):" << mCanopyConductance;
}

/// calculate responses for ground vegetation, i.e. for "unstocked" areas.
/// this duplicates calculations done in Species.
/// @return Minimum of vpd and soilwater response for default
inline double WaterCycle::calculateBaseSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa)
{
    // constant parameters used for ground vegetation:
    const double mPsiMin = 1.5; // MPa
    const double mRespVpdExponent = -0.6;
    // see SpeciesResponse::soilAtmosphereResponses()
    double water_resp;
    // see Species::soilwaterResponse:
    const double psi_mpa = psi_kpa / 1000.; // convert to MPa
    water_resp = limit( 1. - psi_mpa / mPsiMin, 0., 1.);
    // see species::vpdResponse

    double vpd_resp;
    vpd_resp =  exp(mRespVpdExponent * vpd_kpa);
    return qMin(water_resp, vpd_resp);
}

/// calculate combined VPD and soilwaterresponse for all species
/// on the RU. This is used for the calc. of the transpiration.
inline double WaterCycle::calculateSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa)
{
    double min_response;
    double total_response = 0; // LAI weighted minimum response for all speices on the RU
    double total_lai_factor = 0.;
    foreach(const ResourceUnitSpecies *rus, mRU->ruSpecies()) {
        if (rus->LAIfactor()>0.) {
            // retrieve the minimum of VPD / soil water response for that species
            rus->speciesResponse()->soilAtmosphereResponses(psi_kpa, vpd_kpa, min_response);
            total_response += min_response * rus->LAIfactor();
            total_lai_factor += rus->LAIfactor();
        }
    }

    if (total_lai_factor<1.) {
        // the LAI is below 1: the rest is considered as "ground vegetation"
        total_response += calculateBaseSoilAtmosphereResponse(psi_kpa, vpd_kpa) * (1. - total_lai_factor);
    }

    // add an aging factor to the total response (averageAging: leaf area weighted mean aging value):
    // conceptually: response = min(vpd_response, water_response)*aging
    if (total_lai_factor==1.)
        total_response *= mRU->averageAging(); // no ground cover: use aging value for all LA
    else if (total_lai_factor>0. && mRU->averageAging()>0.)
        total_response *= (1.-total_lai_factor)*1. + (total_lai_factor * mRU->averageAging()); // between 0..1: a part of the LAI is "ground cover" (aging=1)

    DBGMODE(
          if (mRU->averageAging()>1. || mRU->averageAging()<0. || total_response<0 || total_response>1.)
             qDebug() << "water cycle: average aging invalid. aging:" << mRU->averageAging() << "total response" << total_response << "total lai factor:" << total_lai_factor;
    );

    //DBG_IF(mRU->averageAging()>1. || mRU->averageAging()<0.,"water cycle", "average aging invalid!" );
    return total_response;
}


/// Main Water Cycle function. This function triggers all water related tasks for
/// one simulation year.
/// @sa http://iland.boku.ac.at/water+cycle
void WaterCycle::run()
{
    // necessary?
    if (GlobalSettings::instance()->currentYear() == mLastYear)
        return;
    DebugTimer tw("water:run");
    WaterCycleData add_data;

    // preparations (once a year)
    getStandValues(); // fetch canopy characteristics from iLand (including weighted average for mCanopyConductance)
    mCanopy.setStandParameters(mLAINeedle,
                               mLAIBroadleaved,
                               mCanopyConductance);

    // main loop over all days of the year
    double prec_mm, prec_after_interception, prec_to_soil, et, excess;
    const Climate *climate = mRU->climate();
    const ClimateDay *day = climate->begin();
    const ClimateDay *end = climate->end();
    int doy=0;
    double total_excess = 0.;
    for (; day<end; ++day, ++doy) {
        // (1) precipitation of the day
        prec_mm = day->preciptitation;
        // (2) interception by the crown
        prec_after_interception = mCanopy.flow(prec_mm);
        // (3) storage in the snow pack
        prec_to_soil = mSnowPack.flow(prec_after_interception, day->temperature);
        // save extra data (used by e.g. fire module)
        add_data.water_to_ground[doy] = prec_to_soil;
        add_data.snow_cover[doy] = mSnowPack.snowPack();
        // (4) add rest to soil
        mContent += prec_to_soil;

        excess = 0.;
        if (mContent>mFieldCapacity) {
            // excess water runoff
            excess = mContent - mFieldCapacity;
            total_excess += excess;
            mContent = mFieldCapacity;
        }

        double current_psi = psiFromHeight(mContent);
        mPsi[doy] = current_psi;

        // (5) transpiration of the vegetation (and of water intercepted in canopy)
        // calculate the LAI-weighted response values for soil water and vpd:
        double interception_before_transpiration = mCanopy.interception();
        double combined_response = calculateSoilAtmosphereResponse( current_psi, day->vpd);
        et = mCanopy.evapotranspiration3PG(day, climate->daylength_h(doy), combined_response);
        // if there is some flow from intercepted water to the ground -> add to "water_to_the_ground"
        if (mCanopy.interception() < interception_before_transpiration)
            add_data.water_to_ground[doy]+= interception_before_transpiration - mCanopy.interception();

        mContent -= et; // reduce content (transpiration)
        // add intercepted water (that is *not* evaporated) again to the soil (or add to snow if temp too low -> call to snowpack)
        mContent += mSnowPack.add(mCanopy.interception(),day->temperature);
        
        // do not remove water below the PWP (fixed value)
        if (mContent<mPermanentWiltingPoint) {
            et -= mPermanentWiltingPoint - mContent; // reduce et (for bookkeeping)
            mContent = mPermanentWiltingPoint;
        }


        //DBGMODE(
            if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dWaterCycle)) {
                DebugList &out = GlobalSettings::instance()->debugList(day->id(), GlobalSettings::dWaterCycle);
                // climatic variables
                out << day->id() << mRU->index() << mRU->id() << day->temperature << day->vpd << day->preciptitation << day->radiation;
                out << combined_response; // combined response of all species on RU (min(water, vpd))
                // fluxes
                out << prec_after_interception << prec_to_soil << et << mCanopy.evaporationCanopy()
                        << mContent << mPsi[doy] << excess;
                // other states
                out << mSnowPack.snowPack();
                //special sanity check:
                if (prec_to_soil>0. && mCanopy.interception()>0.)
                    if (mSnowPack.snowPack()==0. && day->preciptitation==0)
                        qDebug() << "watercontent increase without precipititaion";

            }
        //); // DBGMODE()

    }
    // call external modules
    GlobalSettings::instance()->model()->modules()->calculateWater(mRU, &add_data);
    mLastYear = GlobalSettings::instance()->currentYear();

}


namespace Water {

/** calculates the input/output of water that is stored in the snow pack.
  The approach is similar to Picus 1.3 and ForestBGC (Running, 1988).
  Returns the amount of water that exits the snowpack (precipitation, snow melt) */
double SnowPack::flow(const double &preciptitation_mm, const double &temperature)
{
    if (temperature>mSnowTemperature) {
        if (mSnowPack==0.)
            return preciptitation_mm; // no change
        else {
            // snow melts
            const double melting_coefficient = 0.7; // mm/°C
            double melt = qMin( (temperature-mSnowTemperature) * melting_coefficient, mSnowPack);
            mSnowPack -=melt;
            return preciptitation_mm + melt;
        }
    } else {
        // snow:
        mSnowPack += preciptitation_mm;
        return 0.; // no output.
    }

}


inline double SnowPack::add(const double &preciptitation_mm, const double &temperature)
{
    // do nothing for temps > 0°
    if (temperature>mSnowTemperature)
        return preciptitation_mm;

    // temps < 0°: add to snow
    mSnowPack += preciptitation_mm;
    return 0.;
}

/** Interception in crown canopy.
    Calculates the amount of preciptitation that does not reach the ground and
    is stored in the canopy. The approach is adopted from Picus 1.3.
    Returns the amount of precipitation (mm) that surpasses the canopy layer.
    @sa http://iland.boku.ac.at/water+cycle#precipitation_and_interception */
double Canopy::flow(const double &preciptitation_mm)
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
        double max_storage_needle = mNeedleFactor * (1. - exp(-0.55*mLAINeedle) );
        max_storage_mm += max_storage_needle;
    }

    if (mLAIBroadleaved>0.) {
        // (1) calculate maximum fraction of thru-flow the crown (based on precipitation)
        double max_flow_broad = 0.9 * pow(1.22 - exp(-0.055*preciptitation_mm), 0.35);
        max_interception_mm += preciptitation_mm *  (1. - max_flow_broad * mLAIBroadleaved/mLAI);
        // (2) calculate maximum storage potential based on the current LAI
        double max_storage_broad = mDecidousFactor * (1. - exp(-0.5*mLAIBroadleaved) );
        max_storage_mm += max_storage_broad;
    }

    // (3) calculate actual interception and store for evaporation calculation
    mInterception = qMin( max_storage_mm, max_interception_mm );

    // (4) limit interception with amount of precipitation
    mInterception = qMin( mInterception, preciptitation_mm);

    // (5) reduce preciptitaion by the amount that is intercepted by the canopy
    return preciptitation_mm - mInterception;

}

/// sets up the canopy. fetch some global parameter values...
void Canopy::setup()
{
    mAirDensity = Model::settings().airDensity; // kg / m3
}

void Canopy::setStandParameters(const double LAIneedle, const double LAIbroadleave, const double maxCanopyConductance)
{
    mLAINeedle = LAIneedle;
    mLAIBroadleaved=LAIbroadleave;
    mLAI=LAIneedle+LAIbroadleave;
    mAvgMaxCanopyConductance = maxCanopyConductance;

    // clear aggregation containers
    for (int i=0;i<12;++i) mET0[i]=0.;

}



/** calculate the daily evaporation/transpiration using the Penman-Monteith-Equation.
   This version is based on 3PG. See the Visual Basic Code in 3PGjs.xls.
   Returns the total sum of evaporation+transpiration in mm of the day. */
double Canopy::evapotranspiration3PG(const ClimateDay *climate, const double daylength_h, const double combined_response)
{
    double vpd_mbar = climate->vpd * 10.; // convert from kPa to mbar
    double temperature = climate->temperature; // average temperature of the day (°C)
    double daylength = daylength_h * 3600.; // daylength in seconds (convert from length in hours)
    double rad = climate->radiation / daylength * 1000000; //convert from MJ/m2 (day sum) to average radiation flow W/m2 [MJ=MWs -> /s * 1,000,000

    // the radiation: based on linear empirical function
    const double qa = -90.;
    const double qb = 0.8;
    double net_rad = qa + qb*rad;

    //: Landsberg original: const double e20 = 2.2;  //rate of change of saturated VP with T at 20C
    const double VPDconv = 0.000622; //convert VPD to saturation deficit = 18/29/1000
    const double latent_heat = 2460000.; // Latent heat of vaporization. Energy required per unit mass of water vaporized [J kg-1]

    double gBL  = Model::settings().boundaryLayerConductance; // boundary layer conductance

    // canopy conductance.
    // The species traits are weighted by LAI on the RU.
    // maximum canopy conductance: see getStandValues()
    // current response: see calculateSoilAtmosphereResponse(). This is basically a weighted average of min(water_response, vpd_response) for each species
    double gC = mAvgMaxCanopyConductance * combined_response;


    double defTerm = mAirDensity * latent_heat * (vpd_mbar * VPDconv) * gBL;

    //  with temperature-dependent  slope of  vapor pressure saturation curve
    // (following  Allen et al. (1998),  http://www.fao.org/docrep/x0490e/x0490e07.htm#atmospheric%20parameters)
    // svp_slope in mbar.
    //double svp_slope = 4098. * (6.1078 * exp(17.269 * temperature / (temperature + 237.3))) / ((237.3+temperature)*(237.3+temperature));

    // alternatively: very simple variant (following here the original 3PG code). This
    // keeps yields +- same results for summer, but slightly lower values in winter (2011/03/16)
    double svp_slope = 2.2;

    double div = (1. + svp_slope + gBL / gC);
    double Etransp = (svp_slope * net_rad + defTerm) / div;
    double canopy_transpiration = Etransp / latent_heat * daylength;

    // calculate reference evapotranspiration
    // see Adair et al 2008
    const double psychrometric_const = 0.0672718682328237; // kPa/degC
    const double windspeed = 2.; // m/s
    double net_rad_mj_day = net_rad*daylength/1000000.; // convert W/m2 again to MJ/m2*day
    double et0_day = 0.408*svp_slope*net_rad_mj_day  + psychrometric_const*900./(temperature+273.)*windspeed*climate->vpd;
    double et0_div = svp_slope+psychrometric_const*(1.+0.34*windspeed);
    et0_day = et0_day / et0_div;
    mET0[climate->month-1] += et0_day;

    if (mInterception>0.) {
        // we assume that for evaporation from leaf surface gBL/gC -> 0
        double div_evap = 1. + svp_slope;
        double evap_canopy_potential = (svp_slope*net_rad + defTerm) / div_evap / latent_heat * daylength;
        // reduce the amount of transpiration on a wet day based on the approach of
        // Wigmosta et al (1994). see http://iland.boku.ac.at/water+cycle#transpiration_and_canopy_conductance

        double ratio_T_E = canopy_transpiration / evap_canopy_potential;
        double evap_canopy = qMin(evap_canopy_potential, mInterception);

        // for interception -> 0, the canopy transpiration is unchanged
        canopy_transpiration = (evap_canopy_potential - evap_canopy) * ratio_T_E;

        mInterception -= evap_canopy; // reduce interception
        mEvaporation = evap_canopy; // evaporation from intercepted water

    }
    return canopy_transpiration;
}

} // end namespace
