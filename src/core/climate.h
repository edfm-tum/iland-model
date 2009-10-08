#ifndef CLIMATE_H
#define CLIMATE_H

#include <QtSql>
#include "phenology.h"
/// current climate variables of a day. @sa Climate.
struct ClimateDay
{
    int year; // year
    int month; // month
    int day; // day of year
    double temperature; // average day °C
    double temp_delayed; // temperature delayed (after Maekela, 2008) for response calculations
    double preciptitation; // sum of day [mm]
    double radiation; // sum of day (MJ/m2)
    double vpd; // average of day [kPa] = [0.1 mbar] (1 bar = 100kPa)
    static double co2; // ambient CO2 content in ppm
    QString toString() const { return QString("%1.%2.%3").arg(day).arg(month).arg(year); }
    bool isValid() const  { return (year>=0); }
    int id() const { return year*10000 + month*100 + day; }
};

/// Sun handles calculations of day lengths, etc.
class Sun
{
public:
    void setup(const double latitude_rad);
    QString dump();
    const double &daylength(const int day) const { return mDaylength_h[day]; }
    int longestDay() const { return mDayWithMaxLength; }
    bool northernHemishere() const { return mDayWithMaxLength<300; }
private:
    double mLatitude; ///< latitude in radians
    int mDayWithMaxLength; ///< day of year with maximum day length
    double mDaylength_h[366]; ///< daylength per day in hours
};

class Climate
{
public:
    Climate();
    void setup(); ///< setup routine that opens database connection
    // activity
    void nextYear();
    // access to climate data
    const ClimateDay *dayOfYear(const int dayofyear) { return mBegin + dayofyear;} ///< get pointer to climate structure by day of year (0-based-index)
    const ClimateDay *day(const int month, const int day); ///< gets pointer to climate structure of given day (0-based indices, i.e. month=11=december!)
    /// returns two pointer (arguments!!!) to the begin and one after end of the given month (month: 0..11)
    void monthRange(const int month, const ClimateDay **rBegin, const ClimateDay **rEnd);
    double days(const int month); ///< returns number of days of given month
    int daysOfYear(); ///< returns number of days of current year. points to the first day of the current year.
    const ClimateDay *begin() { return mBegin; } ///< STL-like (pointer)-iterator to the day *after* last day of the current year
    const ClimateDay *end() { return mEnd; } ///< STL-like pointer iterator
    void toDate(const int yearday, int *rDay=0, int *rMonth=0, int *rYear=0); ///< decode "yearday" to the actual year, month, day if provided
    // access to other subsystems
    const Phenology &phenology(const int phenologyGroup) const; ///< phenology class of given type
    const Sun &sun() const { return mSun; } ///< solar radiation class
    double daylength_h(const int doy) const { return sun().daylength(doy); } ///< length of the day in hours

private:
    Sun mSun; ///< class doing solar radiation calculations
    void load(); ///< load mLoadYears years from database
    void setupPhenology(); ///< setup of phenology groups
    void climateCalculations(ClimateDay &lastDay); ///< more calculations done after loading of climate data
    ClimateDay mInvalidDay;
    int mLoadYears; // count of years to load ahead
    int mCurrentYear; // current year (relative)
    int mMinYear; // lowest year in store (relative)
    int mMaxYear;  // highest year in store (relative)
    ClimateDay *mBegin; // pointer to the first day of the current year
    ClimateDay *mEnd; // pointer to the last day of the current year (+1)
    QVector<ClimateDay> mStore; ///< storage of climate data
    QVector<int> mDayIndices; ///< store indices for month / years within store
    QSqlQuery mClimateQuery; ///< sql query for db access
    QList<Phenology> mPhenology; ///< phenology calculations
};

#endif // CLIMATE_H
