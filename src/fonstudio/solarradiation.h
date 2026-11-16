#ifndef SolarRadiation_H
#define SolarRadiation_H

#include <math.h>

class HemiGrid;
/** Handles sun and radiation calculations.
  This class contains functions to calculate hemispherical radiation intensities
  based on latitude. */
class SolarRadiation
{
public:
    SolarRadiation(): Latitude(0.), mMinDay(-1), mMaxDay(367), mDiffusRadFraction(0.5) {}

    /** calculate radiation matrix.
      calculates for each sector of the "Grid" the yearly radiation intensities.
      Intensities are influenced by latitude, vegetation period and the fraction of diffuse radiation.
      @param Step_deg size of one cell in degree (e.g. 5 -> each pixel has a size of 5x5 deg)
      @param Grid results are stored in that HemiGrid (no setup required) */
    void calculateRadMatrix(const float Step_deg, HemiGrid &Grid);
    /// set fraction of diffuse radiation (1: only diffuse rad, 0: only direct rad)
    void setDiffusRadFraction(double fraction_of_diffus_rad) { mDiffusRadFraction = fraction_of_diffus_rad; }
    /// set latitude in degree
    void setLatidude(const double lat_degree) { Latitude = lat_degree * M_PI/180.; }
    void setVegetationPeriod(int day_start, int day_end) { mMinDay=day_start; mMaxDay=day_end; }
private:
    double Latitude;    ///< Latitude in rad
    int mMinDay; ///< start of vegetation period (day)
    int mMaxDay; ///< end of vegetation period (day)
    double mDiffusRadFraction;
};

#endif
