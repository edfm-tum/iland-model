/** @class Climate encapsulates calculations related to a climate dataset.
*/
#include "global.h"
#include "climate.h"
#include "model.h"

Climate::Climate()
{
    mLoadYears = 1;
    mInvalidDay.day=mInvalidDay.month=mInvalidDay.year=-1;
}


// access functions
const ClimateDay *Climate::dayOfYear(const int dayofyear)
{
    return mStore.constBegin() + mDayIndices[mCurrentYear*12]+ dayofyear;
}
const ClimateDay *Climate::day(const int month, const int day)
{
    if (mDayIndices.isEmpty())
        return &mInvalidDay;
    return mStore.constBegin() + mDayIndices[mCurrentYear*12 + month] + day;
}
void Climate::monthRange(const int month, const ClimateDay **rBegin, const ClimateDay **rEnd)
{
    *rBegin = mStore.constBegin() + mDayIndices[mCurrentYear*12 + month];
    *rEnd = mStore.constBegin() + mDayIndices[mCurrentYear*12 + month+1];
    qDebug() << "monthRange returning: begin:"<< (*rBegin)->date() << "end-1:" << (*rEnd-1)->date();
}

double Climate::days(const int month)
{
    return (double) mDayIndices[mCurrentYear*12 + month + 1]-mDayIndices[mCurrentYear*12 + month];
}
int Climate::daysOfYear()
{
    if (mDayIndices.isEmpty())
        return -1;
    return mDayIndices[(mCurrentYear+1)*12]-mDayIndices[mCurrentYear*12];
}


void Climate::setup()
{
    GlobalSettings *g=GlobalSettings::instance();
    XmlHelper xml(g->settings().node("model.climate"));
    QString tableName =xml.value("tableName");
    mLoadYears = (int) xml.valueDouble("batchYears", 1.);
    mStore.resize(mLoadYears * 366);
    mCurrentYear=0;
    mMinYear = 0;
    mMaxYear = 0;

    // setup query
    QString query=QString("select year,month,day,temp,prec,rad,vpd from %1 order by year, month, day").arg(tableName);
    // here add more options...
    mClimateQuery = QSqlQuery(g->dbclimate());
    mClimateQuery.exec(query);
    if (mClimateQuery.lastError().isValid()){
        throw IException(QString("Error setting up climate: %1 \n %2").arg(query, mClimateQuery.lastError().text()) );
    }
    // load first chunk...
    load();
}


void Climate::load()
{
    if (!mClimateQuery.isActive())
       throw IException(QString("Error loading climate file - query not active."));

    ClimateDay lastDay = *day(11,30); // 31.december
    mMinYear = mMaxYear;
    QVector<ClimateDay>::iterator store=mStore.begin();

    mDayIndices.clear();
    ClimateDay *cday = store;
    int lastmon = -1;
    int lastyear = -1;
    int yeardays;
    for (int i=0;i<mLoadYears;i++) {
        yeardays = 0;
        //qDebug() << "loading year" << lastyear+1;
        while(1==1) {
            if(!mClimateQuery.next()) {
                // rewind to start
                qDebug() << "restart of climate table";
                lastyear=-1;
                if (!mClimateQuery.first())
                    throw IException("Error rewinding climate file!");
            }
            yeardays++;
            if (yeardays>366)
                throw IException("Error in reading climate file: yeardays>366!");


            cday = store++; // store values directly in the QVector

            cday->year = mClimateQuery.value(0).toInt();
            cday->month = mClimateQuery.value(1).toInt();
            cday->day = mClimateQuery.value(2).toInt();
            cday->temperature = mClimateQuery.value(3).toDouble();
            cday->preciptitation = mClimateQuery.value(4).toDouble();
            cday->radiation = mClimateQuery.value(5).toDouble();
            cday->vpd = mClimateQuery.value(6).toDouble();
            if (cday->month != lastmon) {
                // new month...
                lastmon = cday->month;
                // save relative position of the beginning of the new month
                mDayIndices.push_back( cday - mStore.constBegin() );
            }
            if (yeardays==1) {
                // check on first day of the year
                if (lastyear!=-1 && cday->year!=lastyear+1)
                    throw IException(QString("Error in reading climate file: invalid year break at y-m-d: %1-%2-%3!").arg(cday->year).arg(cday->month).arg(cday->day));
            }
            if (cday->month==12 && cday->day==31)
                break;

            if (cday==mStore.end())
                throw IException("Error in reading climate file: read across the end!");
        }
        lastyear = cday->year;
    }
    while (store!=mStore.end())
        *store++ = mInvalidDay; // save a invalid day at the end...

    mDayIndices.push_back(cday- mStore.begin()); // the absolute last day...
    mMaxYear = mMinYear+mLoadYears;
    mCurrentYear = 0;
    climateCalculations(lastDay);

}


void Climate::nextYear()
{

    if (mCurrentYear >= mLoadYears-1) // overload
        load();
    else
        mCurrentYear++;
    //qDebug() << "current year is" << mMinYear + mCurrentYear;
}

void Climate::climateCalculations(ClimateDay &lastDay)
{
    ClimateDay *c = mStore.begin();
    const double tau = Model::settings().temperatureTau;
    // handle first day: use tissue temperature of the last day of the last year (if available)
    if (lastDay.isValid())
        c->temp_delayed = lastDay.temp_delayed + 1./tau * (c->temperature - lastDay.temp_delayed);
    else
        c->temp_delayed = c->temperature;
    c++;
    while (c->isValid()) {
        // first order dynamic delayed model (Mäkela 2008)
        c->temp_delayed=(c-1)->temp_delayed + 1./tau * (c->temperature - (c-1)->temp_delayed);
        ++c;
    }

}
