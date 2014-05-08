#include "abe_global.h"
#include "actplanting.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fmtreelist.h"
#include "forestmanagementengine.h"

#include "model.h"
#include "mapgrid.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "sapling.h"

namespace ABE {

ActPlanting::ActPlanting(FMSTP *parent): Activity(parent)
{
    //mBaseActivity.setIsScheduled(true); // use the scheduler
    //mBaseActivity.setDoSimulate(true); // simulate per default

}

void ActPlanting::setup(QJSValue value)
{
    Activity::setup(value); // setup base events

    QJSValue items = FMSTP::valueFromJs(value, "items");
    mItems.clear();
    // iterate over array or over single object
    if ((items.isArray() || items.isObject()) && !items.isCallable()) {
        QJSValueIterator it(items);
        while (it.hasNext()) {
            it.next();
            if (it.name()==QStringLiteral("length"))
                continue;

            mItems.append(SPlantingItem());
            SPlantingItem &item = mItems.last();
            qDebug() << it.name() << ": " << it.value().toString();
            item.setup(it.value());
        }
    } else {
        mItems.append(SPlantingItem());
        SPlantingItem &item = mItems.last();
        item.setup(items);
    }
}

bool ActPlanting::execute(FMStand *stand)
{
    qCDebug(abe) << stand->context() << "execute of planting activity....";
    QRectF box = ForestManagementEngine::instance()->standGrid()->boundingBox(stand->id());
    const MapGrid *sgrid = ForestManagementEngine::instance()->standGrid();
    Model *model = GlobalSettings::instance()->model();
    GridRunner<float> gr(model->grid(), box);

    // loop over all cells on the LIF grid that are within the bounding box of the stand.
    for (int s=0;s<mItems.count();++s) {
        gr.reset();
        while (float *p=gr.next()) {
            if (sgrid->gridValue(gr.currentIndex()) != stand->id())
                continue;
            //
            if (drandom() < mItems[s].fraction) {
                ResourceUnit *ru = model->ru(gr.currentCoord());
                ResourceUnitSpecies &rus =  ru->resourceUnitSpecies(mItems[s].species);
                rus.addSapling(gr.currentIndex()); // always 5cm
            }
        }
    }


    return true;
}

QStringList ActPlanting::info()
{
    QStringList lines = Activity::info();

    foreach(const SPlantingItem &item, mItems) {
        lines << "-";
        lines << QString("species: %1").arg(item.species->id());
        lines << QString("fraction: %1").arg(item.fraction);
        lines << QString("clear: %1").arg(item.clear);
        lines << "/-";
    }
    return lines;
}


bool ActPlanting::SPlantingItem::setup(QJSValue value)
{
    QString species_id = FMSTP::valueFromJs(value, "species",QString(), "setup of planting item for planting activity.").toString();
    species = GlobalSettings::instance()->model()->speciesSet()->species(species_id);
    if (!species)
        throw IException(QString("'%1' is not a valid species id for setting up a planting item.").arg(species_id));
    fraction = FMSTP::valueFromJs(value, "fraction", "0").toNumber();
    clear = FMSTP::valueFromJs(value, "clear", "false").toBool();
    return true;
}



} // namesapce
