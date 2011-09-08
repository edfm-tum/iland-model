#include "fireout.h"
#include "model.h"
#include "resourceunit.h"
#include "snag.h"
#include "soil.h"

FireOut::FireOut()
{
    mFire = 0;
    setName("Fire RU/yr", "fire");
    setDescription("Fire event aggregates per fire event. "\
                   "The output contains a row for each (ignited) fire event. " \
                   " ");
    columns() << OutputColumn::year()
              << OutputColumn("area_plan_m2", "Area of the planned fire m2", OutInteger)
              << OutputColumn("area_m2", "Realized area of burnt cells m2", OutInteger)
              << OutputColumn("iterations", "Number of iterations of the cellular automaton", OutInteger)
              << OutputColumn("n_trees", "total number of trees on all burning cells", OutInteger)
              << OutputColumn("n_trees_died", "total number of trees that were killed by the fire", OutDouble)
              << OutputColumn("basalArea_died", "sum of basal area of died trees (m2)", OutDouble)
              << OutputColumn("avgFuel_kg_ha", "average total fuel (forest floor + dwd) of burning cells (kg/ha)", OutDouble)
              << OutputColumn("windSpeed", "current wind speed during the event (m/s)", OutDouble)
              << OutputColumn("windDirection", "current wind direction during the event (°)", OutDouble) ;



}

void FireOut::setup()
{
}

// Output function
// fire data is aggregated in this function for each fire event.
void FireOut::exec()
{
    *this << currentYear();
    *this << mFire->fireStats.fire_size_plan_m2 << mFire->fireStats.fire_size_realized_m2;
    *this << mFire->fireStats.iterations;
    *this << mFire->fireStats.startpoint.x() << mFire->fireStats.startpoint.y();
    int fire_id = mFire->mFireId;
    double avg_fuel = 0.;
    int n_ru = 0;
    double n_trees = 0.;
    double n_trees_died = 0.;
    double basal_area = 0.;
    for (FireRUData *fds = mFire->mRUGrid.begin(); fds!=mFire->mRUGrid.end(); ++fds) {
        if (fds->fireRUStats.fire_id == fire_id) {
            // the current fire burnt on this area
            n_ru++;
            avg_fuel += fds->fireRUStats.fuel;
            n_trees += fds->fireRUStats.n_trees;
            n_trees_died += fds->fireRUStats.n_trees_died;
            basal_area += fds->fireRUStats.died_basal_area;
        }
    }
    if (n_ru>0) {
        avg_fuel /= double(n_ru);
    }
    *this << n_trees << n_trees_died << basal_area << avg_fuel;
    *this << mFire->mCurrentWindSpeed << mFire->mCurrentWindDirection;

    writeRow();

}

