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

#ifndef WATERCYCLE_H
#define WATERCYCLE_H
class ResourceUnit;
struct ClimateDay;
class WaterCycle; // forward
/// Water contains helper classes for the water cycle calculations
namespace Water
{


/** SnowPack handles the snow layer.
   @ingroup core
   Snow is conceptually very simple (see http://iland.boku.ac.at/water+cycle).
*/
class SnowPack
{
    friend class ::WaterCycle;
public:
    SnowPack(): mSnowPack(0.) {}
    void setup() { mSnowPack=0.; }
    void setSnow(double snow_mm) { mSnowPack = snow_mm; }
    /// process the snow layer. Returns the mm of preciptitation/melt water that leaves the snow layer.
    double flow(const double &preciptitation_mm, const double &temperature);
    /// additional precipitation (e.g. non evaporated water of canopy interception).
    inline double add(const double &preciptitation_mm, const double &temperature);
    double snowPack() const { return mSnowPack; } ///< current snowpack height (mm)
private:
    double mSnowPack; ///< height of snowpack (mm water column)
    double mSnowTemperature; ///< Threshold temperature for snowing / snow melt

};

/** Canopy handles the the throughfall and evaporation from the forest canopy.
  @ingroup core
  The Canopy class encapsulates the leaves of trees with regard to water related processes. These
  processes are the interception of precipitation, the evaporation from the canopy, and, most important,
  the transpiration of the canopy. The functions are called by WaterCycle.
  */
class Canopy
{

public:
    // setup
    void setup(); ///< setup and load parameter values
    void setStandParameters(const double LAIneedle, const double LAIbroadleave, const double maxCanopyConductance);
    // actions
    /// process the canopy layer. returns the amount of precipitation that leaves the canopy-layer.
    double flow(const double &preciptitation_mm);
    double evapotranspirationBGC(const ClimateDay *climate, const double daylength_h); ///< evapotranspiration from soil
    double evapotranspiration3PG(const ClimateDay *climate, const double daylength_h, const double combined_response); ///< evapotranspiration from soil (mm). returns
    // properties
    double interception() const  { return mInterception; } ///< mm water that is intercepted by the crown
    double evaporationCanopy() const { return mEvaporation; } ///< evaporation from canopy (mm)
    double avgMaxCanopyConductance() const { return mAvgMaxCanopyConductance; } ///< averaged maximum canopy conductance of current species distribution (m/s)
    const double *referenceEvapotranspiration() const { return mET0; } ///< monthly reference ET (see Adair et al 2008)

private:
    double mLAINeedle; // leaf area index of coniferous species
    double mLAIBroadleaved; // leaf area index of broadlevaed species
    double mLAI; // total leaf area index
    double mAvgMaxCanopyConductance; // maximum weighted canopy conductance (m/s)
    double mInterception; ///< intercepted precipitation of the current day (mm)
    double mEvaporation; ///< water that evaporated from foliage surface to atmosphere (mm)
    // Penman-Monteith parameters
    double mAirDensity; // density of air [kg / m3]
    double mET0[12]; ///< reference evapotranspiration per month (sum of the month, mm)
    // parameters for interception
    double mNeedleFactor; ///< factor for calculating water storage capacity for intercepted water for conifers
    double mDecidousFactor; ///< the same for broadleaved
    friend class ::WaterCycle;
    // GCC problems with friend class and namespaces: http://www.ogre3d.org/forums/viewtopic.php?f=10&t=47540
};


} // end namespace Water


class WaterCycle
{
public:
    WaterCycle();
    void setup(const ResourceUnit *ru);
    void setContent(double content, double snow_mm) { mContent = content; mSnowPack.setSnow(snow_mm); }
    // actions
    void run(); ///< run the current year
    // properties
    double fieldCapacity() const { return mFieldCapacity; } ///< field capacity (mm)
    const double &psi_kPa(const int doy) const { return mPsi[doy]; } ///< soil water potential for the day 'doy' (0-index) in kPa
    double soilDepth() const { return mSoilDepth; } ///< soil depth in mm
    double currentContent() const { return mContent; } ///< current water content in mm
    double currentSnowPack() const { return mSnowPack.snowPack(); } ///< current water stored as snow (mm water)
    double canopyConductance() const { return mCanopyConductance; } ///< current canopy conductance (LAI weighted CC of available tree species) (m/s)
    /// monthly values for PET (mm sum)
    const double *referenceEvapotranspiration() const { return mCanopy.referenceEvapotranspiration(); }

private:
    int mLastYear; ///< last year of execution
    inline double psiFromHeight(const double mm) const; // kPa for water height "mm"
    inline double heightFromPsi(const double psi_kpa) const; // water height (mm) at water potential psi (kilopascal)
    inline double calculateBaseSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa); ///< calculate response for ground vegetation
    double mPsi_koeff_b; ///< see psiFromHeight()
    double mPsi_sat; ///< see psiFromHeight(), kPa
    double mTheta_sat; ///< see psiFromHeight(), [-], m3/m3
    const ResourceUnit *mRU; ///< resource unit to which this watercycle is connected
    Water::Canopy mCanopy; ///< object representing the forest canopy (interception, evaporation)
    Water::SnowPack mSnowPack; ///< object representing the snow cover (aggregation, melting)
    double mSoilDepth; ///< depth of the soil (without rocks) in mm
    double mContent; ///< current water content in mm water column of the soil.
    double mFieldCapacity; ///< bucket height of field-capacity (eq. -15kPa) (mm)
    double mPermanentWiltingPoint; ///< bucket "height" of PWP (is fixed to -4MPa) (mm)
    double mPsi[366]; ///< soil water potential for each day in kPa
    void getStandValues(); ///< helper function to retrieve LAI per species group
    inline double calculateSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa);
    double mLAINeedle;
    double mLAIBroadleaved;
    double mCanopyConductance; ///< m/s
};

/// WaterCycleData is a data transfer container for water-related details.
class WaterCycleData
{
public:
    /// daily amount of water that actually reaches the ground (i.e., after interception)
    double water_to_ground[366];
    /// height of snow cover [mm water column]
    double snow_cover[366];
};

#endif // WATERCYCLE_H
