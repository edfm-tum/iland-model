#ifndef CLIMATE_H
#define CLIMATE_H

#include <QtSql>

struct ClimateDay
{
    int year; // year
    int month; // month
    int day; // day of year
    double temperature; // average day °C
    double preciptitation; // sum of day [mm]
    double radiation; // sum of day (MJ/m2)
    double vpd; // average of day [kPa]
};
class Climate
{
public:
    Climate();
    void setup(); ///< setup routine that opens database connection
    // activity
    void nextYear();

private:
    void load(); ///< load mLoadYears years from database
    int mLoadYears; // count of years to load ahead
    int mCurrentYear; // current year (relative)
    int mMinYear; // lowest year in store (relative)
    int mMaxYear;  // highest year in store (relative)
    QVector<ClimateDay> mStore; ///< storage of climate data
    QVector<int> mDayIndices; ///< store indices for month / years within store
    QSqlQuery mClimateQuery; ///< sql query for db access
};

#endif // CLIMATE_H
