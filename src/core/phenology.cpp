#include "global.h"
#include "phenology.h"

#include "climate.h"
#include "floatingaverage.h"

/// "ramp" function: value < min: 0, value>max: 1: in between linear interpolation
double ramp(const double &value, const double minValue, const double maxValue)
{
    Q_ASSERT(minValue!=maxValue);
    if (value<minValue) return 0.;
    if (value>maxValue) return 1.;
    return (value-minValue) / (maxValue - minValue);
}

/** calculates the phenology according to Jolly et al. 2005.
  The calculation is performed for a given "group" and a present "climate".
*/
void Phenology::calculate()
{
    if (id()==0) {
        // for needles: just calculate the chilling requirement for the establishment
        // i.e.: use the "bottom line" of 10.5 hrs daylength for the end of the vegetation season
        calculateChillDays(mClimate->sun().dayShorter10_5hrs());
        return;
    }
    const ClimateDay  *end = mClimate->end();
    double vpd,temp,daylength;
    double gsi; // combined factor of effect of vpd, temperature and day length
    int iday = 0;
    bool inside_period = !mClimate->sun().northernHemishere(); // on northern hemisphere 1.1. is in winter
    int day_start=-1, day_stop=-1;
    int day_wait_for=-1;

    FloatingAverage floater(21); // a three-week floating average
    for (const ClimateDay *day = mClimate->begin(); day!=end; ++day, ++iday) {

        if (day_wait_for>=0 && iday<day_wait_for)
            continue;
        vpd = 1. - ramp(day->vpd, mMinVpd, mMaxVpd); // high value for low vpd
        temp = ramp(day->min_temperature, mMinTemp, mMaxTemp);
        daylength = ramp( mClimate->sun().daylength(iday), mMinDayLength, mMaxDayLength);
        gsi = vpd * temp * daylength;
        gsi = floater.add(gsi);
        if (!inside_period && gsi>0.5) {
            // switch from winter -> summer
            inside_period = true;
            day_start = iday;
            if (day_stop!=-1)
                break;
            day_wait_for = mClimate->sun().longestDay();
        } else if (inside_period && gsi<0.5) {
            // switch from summer to winter
            day_stop = iday;
            if (day_start!=-1)
                break; // finished
            day_wait_for = mClimate->sun().longestDay();
            inside_period = false;
        }
    }
    day_start -= 10; // three-week-floating average: subtract 10 days
    day_stop -= 10;
    if (day_start < -1 || day_stop<-1)
        throw IException(QString("Phenology::calculation(): was not able to determine the length of the vegetation period for group %1." ).arg(id()));
    if (logLevelDebug())
        qDebug() << "Jolly-phenology. start" << mClimate->dayOfYear(day_start)->toString() << "stop" << mClimate->dayOfYear(day_stop)->toString();
    iday = 0;
    mDayStart = day_start;
    mDayEnd = day_stop;
    int bDay, bMon, eDay, eMon;
    // convert yeardays to dates
    mClimate->toDate(day_start, &bDay, &bMon);
    mClimate->toDate(day_stop, &eDay, &eMon);
    for (int i=0;i<12;i++) {
        if (i<bMon || i>eMon) {
            mPhenoFraction[i] = 0; // out of season
        } else if (i>bMon && i<eMon) {
            mPhenoFraction[i] = 1.; // full inside of season
        } else {
            // fractions of month
            mPhenoFraction[i] = 1.;
            if (i==bMon)
                mPhenoFraction[i] -=  (bDay+1) / double(mClimate->days(bMon));
            if (i==eMon)
                mPhenoFraction[i] -=  (mClimate->days(eMon) - (bDay+1)) / double(mClimate->days(eMon));
        }
    }

    calculateChillDays();
}


// *********** Chill-day calculations ********
void Phenology::calculateChillDays(const int end_of_season)
{
    int iday = 0;
    mChillDaysBefore = 0;
    int days_after = 0;
    int last_day = end_of_season>0?end_of_season:mDayEnd;
    for (const ClimateDay *day = mClimate->begin(); day!=mClimate->end(); ++day, ++iday) {
        if (day->temperature>=-5 && day->temperature<5) {
            if (iday<mDayStart)
                mChillDaysBefore++;
            if (iday>last_day)
                days_after++;
        }
    }
    if (GlobalSettings::instance()->currentYear()==1) {
        // for the first simulation year, use the value of this autumn for the last years autumn
        mChillDaysAfterLastYear = days_after;
    } else {
        mChillDaysAfterLastYear  = mChillDaysAfter;
    }
    mChillDaysAfter = days_after;
}