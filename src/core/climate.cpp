/** @class Climate encapsulates calculations related to a climate dataset.
*/
#include "global.h"
#include "climate.h"

Climate::Climate()
{
    mLoadYears = 1;
}


void Climate::setup()
{
    GlobalSettings *g=GlobalSettings::instance();
    XmlHelper xml(g->settings().node("model.climate"));
    QString tableName =xml.value("tableName");
    mLoadYears = (int) xml.valueDouble("batchYears", 1.);
    mStore.reserve(mLoadYears * 366);
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
}


void Climate::load()
{
    if (!mClimateQuery.isActive())
       throw IException(QString("Error loading climate file - query not active."));

    mMinYear = mMaxYear + 1;
    QVector<ClimateDay>::iterator store=mStore.begin();
    mDayIndices.clear();
    ClimateDay *cday = store;
    int lastmon = -1;
    int lastyear = -1;
    int yeardays;
    for (int i=0;i<mLoadYears;i++) {
        yeardays = 0;
        qDebug() << "loading year" << lastyear+1;
        if(!mClimateQuery.next()) {
            // rewind to start
            qDebug() << "restart of climate table";
            if (!mClimateQuery.first())
                throw IException("Error rewinding climate file!");

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
                mDayIndices.push_back( cday - mStore.begin() );
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
    mMaxYear = mMinYear+mLoadYears;
    mCurrentYear = 0;
}


void Climate::nextYear()
{
    mCurrentYear++;
    if (mCurrentYear >= mMaxYear) // overload
        load();
    qDebug() << "current year is" << mMinYear + mCurrentYear;
}
