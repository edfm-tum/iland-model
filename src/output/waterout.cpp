#include "waterout.h"
#include "output.h"
#include "model.h"
#include "watercycle.h"
#include "resourceunit.h"
#include "climate.h"

WaterOut::WaterOut()
{
    setName("Water output", "water");
    setDescription("Annual water cycle output on resource unit/landscape unit.\n" \
                   "The output includes annual averages of precipitation, evapotranspiration, water excess, " \
                   "snow cover, and radiation input. The spatial resolution is landscape averages and/or resource unit level (i.e. 100m pixels). " \
                   "Landscape level averages are indicated by -1 for the 'ru' and 'index' columns.\n\n" \
                   "You can specify a 'condition' to limit output execution to specific years (variable 'year'). " \
                   "The 'conditionRU' can be used to suppress resource-unit-level details; eg. specifying 'in(year,100,200,300)' limits output on reosurce unit level to the years 100,200,300 " \
                   "(leaving 'conditionRU' blank enables details per default).");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("stocked_area", "area (ha/ha) which is stocked (covered by crowns, absorbing radiation)", OutDouble)
              << OutputColumn("stockable_area", "area (ha/ha) which is stockable (and within the project area)", OutDouble)
              << OutputColumn("precipitation_mm", "Annual precipitation sum (mm)", OutDouble)
              << OutputColumn("et_mm", "Evapotranspiration (mm)", OutDouble)
              << OutputColumn("excess_mm", "annual sum of water loss due to lateral outflow/groundwater flow (mm)", OutDouble)
              << OutputColumn("snowcover_days", "days with snowcover >0mm", OutInteger)
              << OutputColumn("total_radiation", "total incoming radiation over the year (MJ/m2), sum of data in climate input)", OutDouble)
              << OutputColumn("radiation_snowcover", "sum of radiation input (MJ/m2) for days with snow cover", OutInteger);


}

void WaterOut::exec()
{
    Model *m = GlobalSettings::instance()->model();

    // global condition
    if (!mCondition.isEmpty() && mCondition.calculate(GlobalSettings::instance()->currentYear())==0.)
        return;

    bool ru_level = true;
    // switch off details if this is indicated in the conditionRU option
    if (!mConditionDetails.isEmpty() && mConditionDetails.calculate(GlobalSettings::instance()->currentYear())==0.)
        ru_level = false;


    double ru_count = 0.;
    int snow_days = 0;
    double et=0., excess=0., rad=0., snow_rad=0., p=0.;
    double stockable=0., stocked=0.;
    foreach(ResourceUnit *ru, m->ruList()) {
        if (ru->id()==-1)
            continue; // do not include if out of project area

        const WaterCycle *wc = ru->waterCycle();
        if (ru_level) {
            *this << currentYear() << ru->index() << ru->id();
            *this << ru->stockedArea()/cRUArea << ru->stockableArea()/cRUArea;
            *this << ru->climate()->annualPrecipitation();
            *this << wc->mTotalET << wc->mTotalExcess;
            *this << wc->mSnowDays;
            *this << ru->climate()->totalRadiation() << wc->mSnowRad;
            writeRow();
        }
        ++ru_count;
        stockable+=ru->stockableArea(); stocked+=ru->stockedArea();
        p+=ru->climate()->annualPrecipitation();
        et+=wc->mTotalET; excess+=wc->mTotalExcess; snow_days+=wc->mSnowDays;
        rad+=ru->climate()->totalRadiation();
        snow_rad+=wc->mSnowRad;

    }
    // write landscape sums
    if (ru_count==0.)
        return;
    *this << currentYear() << -1 << -1; // codes -1/-1 for landscape level
    *this << stocked/ru_count/cRUArea << stockable/ru_count/cRUArea;
    *this << p / ru_count; // mean precip
    *this << et / ru_count;
    *this << excess / ru_count;
    *this << snow_days / ru_count;
    *this << rad / ru_count << snow_rad / ru_count;
    writeRow();


}

void WaterOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);

    condition = settings().value(".conditionRU", "");
    mConditionDetails.setExpression(condition);


}
