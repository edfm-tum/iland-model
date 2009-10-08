/** @class Climate encapsulates calculations related to a climate dataset.
*/
#include "global.h"
#include "climate.h"
#include "model.h"

double ClimateDay::co2 = 350.; // base value of ambient CO2-concentration (ppm)


void Sun::setup(double latitude_rad)
{
    mLatitude = latitude_rad;
    if (mLatitude>0)
        mDayWithMaxLength = 182-10; // 21.juni
    else
        mDayWithMaxLength = 365-10; //southern hemisphere
    // calculate length of day using  the approximation formulae of: http://herbert.gandraxa.com/length_of_day.aspx
    const double j = M_PI / 182.625;
    const double ecliptic = RAD(23.439);
    double m;
    for (int day=0;day<366;day++) {
        m = 1. - tan(latitude_rad)*tan(ecliptic*cos(j*(day+10))); // day=0: winter solstice => subtract 10 days
        m = limit(m, 0., 2.);
        mDaylength_h[day] = acos(1-m)/M_PI * 24.; // result in hours [0..24]
    }
}

QString Sun::dump()
{
    QString result=QString("lat: %1, longest day: %2\ndoy;daylength").arg(mLatitude).arg(mDayWithMaxLength);
    for (int i=0;i<366;i++)
        result+=QString("\n%1;%2").arg(i).arg(mDaylength_h[i]);
    return result;
}

Climate::Climate()
{
    mLoadYears = 1;
    mInvalidDay.day=mInvalidDay.month=mInvalidDay.year=-1;
    mBegin = mEnd = 0;
}


// access functions

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
    //qDebug() << "monthRange returning: begin:"<< (*rBegin)->toString() << "end-1:" << (*rEnd-1)->toString();
}

double Climate::days(const int month)
{
    return (double) mDayIndices[mCurrentYear*12 + month + 1]-mDayIndices[mCurrentYear*12 + month];
}
int Climate::daysOfYear()
{
    if (mDayIndices.isEmpty())
        return -1;
    return mEnd - mBegin;
}

void Climate::toDate(const int yearday, int *rDay, int *rMonth, int *rYear)
{
    const ClimateDay *d = dayOfYear(yearday);
    if (rDay) *rDay = d->day-1;
    if (rMonth) *rMonth = d->month-1;
    if (rYear) *rYear = d->year;
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
    setupPhenology(); // load phenology
    // setup sun
    mSun.setup(Model::settings().latitude);
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
            // sanity checeks
            DBG_IF(cday->month<1 || cday->day<1 || cday->month>12 || cday->day>31,"Climate:load", "invalid dates");
            DBG_IF(cday->temperature<-70 || cday->temperature>50,"Climate:load", "temperature out of range (-70..+50°C)");
            DBG_IF(cday->preciptitation<0 || cday->preciptitation>50,"Climate:load", "precipitation out of range (0..50mm)");
            DBG_IF(cday->radiation<0 || cday->radiation>50,"Climate:load", "radiation out of range (0..50 MJ/m2/day)");
            DBG_IF(cday->vpd<0 || cday->vpd>10,"Climate:load", "vpd out of range (0..10 kPa)");

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
    mBegin = mStore.begin() + mDayIndices[mCurrentYear*12];
    mEnd = mStore.begin() + mDayIndices[(mCurrentYear+1)*12];; // point to the 1.1. of the next year

    climateCalculations(lastDay); // perform additional calculations based on the climate data loaded from the database

}


void Climate::nextYear()
{

    if (mCurrentYear >= mLoadYears-1) // overload
        load();
    else
        mCurrentYear++;

    mBegin = mStore.begin() + mDayIndices[mCurrentYear*12];
    mEnd = mStore.begin() + mDayIndices[(mCurrentYear+1)*12];; // point to the 1.1. of the next year
    for(int i=0;i<mPhenology.count(); ++i)
        mPhenology[i].calculate();
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


void Climate::setupPhenology()
{
    mPhenology.clear();
    mPhenology.push_back(Phenology()); // id=0
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.species.phenology"));
    int i=0;
    do {
        QDomElement n = xml.node(QString("type[%1]").arg(i));
        if (n.isNull())
            break;
        i++;
        int id;
        id = n.attribute("id", "-1").toInt();
        if (id<0) throw IException(QString("Error setting up phenology: id invalid\ndump: %1").arg(xml.dump("")));
        xml.setCurrentNode(n);
        Phenology item( id,
                        this,
                        xml.valueDouble(".vpdMin",0.5), // use relative access to node (".x")
                        xml.valueDouble(".vpdMax", 5),
                        xml.valueDouble(".dayLengthMin",10),
                        xml.valueDouble(".dayLengthMax",11),
                        xml.valueDouble(".tempMin", 2),
                        xml.valueDouble(".tempMax", 9) );

        mPhenology.push_back(item);
    } while(true);
}

/** return the phenology of the group... */
const Phenology &Climate::phenology(const int phenologyGroup) const
{
    const Phenology &p = mPhenology.at(phenologyGroup);
    if (p.id() == phenologyGroup)
        return p;
    // search...
    for (int i=0;i<mPhenology.count();i++)
        if (mPhenology[i].id()==phenologyGroup)
            return mPhenology[i];
    throw IException(QString("Error at SpeciesSEt::phenology(): invalid group: %1").arg(phenologyGroup));
}
