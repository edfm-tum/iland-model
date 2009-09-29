#ifndef CLIMATE_H
#define CLIMATE_H

#include <QtSql>

struct ClimateDay
{
    int year; // year
    int month; // month
    int day; // day of year
    double temperature; // average day °C
    double temp_delayed; // temperature delayed (after Maekela, 2008) for response calculations
    double preciptitation; // sum of day [mm]
    double radiation; // sum of day (MJ/m2)
    double vpd; // average of day [kPa]
    QString date() const { return QString("%1.%2.%3").arg(day).arg(month).arg(year); }
    bool isValid() const  { return (year>=0); }
};

class Climate
{
public:
    Climate();
    void setup(); ///< setup routine that opens database connection
    // activity
    void nextYear();
    // access to climate data
    const ClimateDay *dayOfYear(const int dayofyear); ///< get pointer to climate structure by day of year (0-based-index)
    const ClimateDay *day(const int month, const int day); ///< gets pointer to climate structure of given day (0-based indices, i.e. month=11=december!)
    /// returns two pointer (arguments!!!) to the begin and one after end of the given month (month: 0..11)
    void monthRange(const int month, const ClimateDay **rBegin, const ClimateDay **rEnd);
    double days(const int month); ///< returns number of days of given month
    int daysOfYear(); ///< returns number of days of current year

private:
    ClimateDay mInvalidDay;
    void load(); ///< load mLoadYears years from database
    void climateCalculations(ClimateDay &lastDay); ///< more calculations done after loading of climate data
    int mLoadYears; // count of years to load ahead
    int mCurrentYear; // current year (relative)
    int mMinYear; // lowest year in store (relative)
    int mMaxYear;  // highest year in store (relative)
    QVector<ClimateDay> mStore; ///< storage of climate data
    QVector<int> mDayIndices; ///< store indices for month / years within store
    QSqlQuery mClimateQuery; ///< sql query for db access
};

#endif // CLIMATE_H
