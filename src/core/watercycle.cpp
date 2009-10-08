#include "global.h"
#include "watercycle.h"
#include "climate.h"
#include "resourceunit.h"
#include "species.h"
#include "xmlhelper.h"

WaterCycle::WaterCycle()
{
    mBucketSize = 0;
}

void WaterCycle::setup(const ResourceUnit *ru, XmlHelper *xml)
{
    mRU = ru;
    // get values...
    mBucketSize = xml->valueDouble("waterHoldingCapacity",100.);
    mContent = mBucketSize / 2.; // start half filled

    mCanopy.setup(xml);
}

void WaterCycle::getStandValues()
{
    mLAIBroadleaved=mLAIBroadleaved=0.;
    mCanopyConductance=0.;
    double lai;
    foreach(const ResourceUnitSpecies &rus, mRU->ruSpecies()) {
        lai = rus.constStatistics().leafAreaIndex();
        if (rus.species()->isConiferous())
            mLAINeedle+=lai;
        else
            mLAIBroadleaved+=lai;
        mCanopyConductance += rus.species()->canopyConductance() * lai;
    }
    double total_lai = mLAIBroadleaved+mLAINeedle;
    if (total_lai>0.) {
        mCanopyConductance /= total_lai;
    }
    if (mCanopyConductance < 3.) {
        // following Landsberg and Waring: when LAI is < 3, a linear "ramp" from 0 to 3 is assumed
        mCanopyConductance *= total_lai / 3.;
    }
    qDebug() << "WaterCycle:getStandValues: LAI needle" << mLAINeedle << "LAI Broadl:"<< mLAIBroadleaved << "weighted avg. Conductance (m/2):" << mCanopyConductance;
}

/// Main Water Cycle function. This function triggers all water related tasks for
/// one simulation year.
void WaterCycle::run()
{
    // preparations (once a year)
    getStandValues(); // fetch canopy characteristics from iLand
    mCanopy.setStandParameters(mLAINeedle,
                               mLAIBroadleaved,
                               mCanopyConductance);


    // main loop over all days of the year
    double prec_mm, prec_after_interception, prec_to_soil, et;
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
        // calculate the relative water content
        mRelativeContent[doy] = relContent();
        // (5) transpiration of the vegetation
        et = mCanopy.evapotranspiration(day, mRU->climate()->daylength_h(doy));

        mContent -= et;
        if (mContent<0.) {
            qDebug() << "water content below zero";
            mContent = 0.;
        }

        if (mContent>mBucketSize) {
            // excess water runoff
            double excess = mContent - mBucketSize;
            total_excess += excess;
            mContent = mBucketSize;
        }
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
            double melt = temperature * melting_coefficient;
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
    Returns the amount of precipitation (mm) that surpasses the canopy layer. */
double Canopy::flow(const double &preciptitation_mm, const double &temperature)
{
    // sanity checks
    mInterception = 0.;
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
        double max_flow_broad = 0.9 * sqrt(1.03 - exp(-0.055*preciptitation_mm));
        max_interception_mm += preciptitation_mm *  (1. - max_flow_broad * mLAIBroadleaved/mLAI);
        // (2) calculate maximum storage potential based on the current LAI
        double max_storage_broad = 4. * (1. - exp(-0.55*mLAIBroadleaved) );
        max_storage_mm += max_storage_broad;
    }

    // (3) calculate actual interception and store for evaporation calculation
    mInterception = qMin( max_storage_mm, max_interception_mm );

    // (4) reduce preciptitaion by the amount that is intercepted by the canopy
    Q_ASSERT(preciptitation_mm > mInterception);
    return preciptitation_mm - mInterception;

}

/// sets up the canopy. expects water-related parameters in the top node of the xml.
void Canopy::setup(XmlHelper *xml)
{
    mMaxCanopyConductance = xml->valueDouble("maxCanopyConductance", 0.02);
    mHeatCapacityAir = xml->valueDouble("heatCapacityAir", 1012);
    mAirDensity = xml->valueDouble("airDensity", 1.204);
    mPsychometricConstant = mHeatCapacityAir*1013./2450000./0.622;
}

void Canopy::setStandParameters(const double LAIneedle, const double LAIbroadleave, const double maxCanopyConductance)
{
    mLAINeedle = LAIneedle;
    mLAIBroadleaved=LAIbroadleave;
    mLAI=LAIneedle+LAIbroadleave;
    mCanopyConductance = maxCanopyConductance;
}

/** calculate the daily evaporation/transpiration using the Penman-Monteith-Equation.
   The application of the equation follows broadly Running (1988).
   Returns the total sum of evaporation+transpiration in mm of the day. */
double Canopy::evapotranspiration(const ClimateDay *climate, const double daylength_h)
{
    double vpd_mbar = climate->vpd * 10.; // convert from kPa to mbar
    double temperature = climate->temperature; // average temperature of the day (°C)
    double daylength = daylength_h * 3600.; // daylength in seconds (convert from length in hours)
    double rad = climate->radiation / daylength * 1000000; //convert from MJ/m2 (day sum) to average radiation flow W/m2 [MJ=MWs -> /s * 1,000,000
    const double aerodynamic_resistance = 5.; // m/s: aerodynamic resistance of the canopy is considered being constant
    const double latent_heat = 2257000.; // Latent heat of vaporization. Energy required per unit mass of water vaporized [J kg-1]

    // (1) calculate some intermediaries
    // current canopy conductance: is calculated following Landsberg & Waring
    double current_canopy_conductance;

    current_canopy_conductance = mMaxCanopyConductance * exp(-2.5 * vpd_mbar);
    // saturation vapor pressure (Running 1988, Eq. 1)
    double svp = 6.1078 * exp((17.269 * temperature) / (237.3 + temperature) );
    // the slope of svp is, thanks to http://www.wolframalpha.com/input/?i=derive+y%3D6.1078+exp+((17.269x)/(237.3%2Bx))
    double svp_slope = svp * ( 17.269/(237.3+temperature) - 17.269*temperature/((237.3+temperature)*(237.3+temperature)) );

    //=((((C37*C24)+(C31*C32)*C25/C33)/(C37+C34*(1+1/C29/C33)))/(C35*1000))*C26*C27
    double et;
    double dim = svp_slope + mPsychometricConstant*(1. + mCanopyConductance / aerodynamic_resistance);
    double dayl = 86400 * latent_heat;
    double upper = svp_slope*rad + mHeatCapacityAir*mAirDensity * vpd_mbar/aerodynamic_resistance;
    et = upper / dim * dayl;
    return et;
}


} // end namespace
