#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"
#include "debugtimer.h"

BarkBeetleModule::BarkBeetleModule()
{
    mLayers.setGrid(mGrid);
    mRULayers.setGrid(mRUGrid);

}

void BarkBeetleModule::setup()
{
    // setup the wind grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    BarkBeetleCell cell;
    mGrid.initialize(cell);

    mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(), GlobalSettings::instance()->model()->RUgrid().cellsize());
    BarkBeetleRUCell ru_cell;
    mRUGrid.initialize(ru_cell);

    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.barkbeetle"));
    qDebug() << "BBSETUP TEST:" << xml.valueDouble(".test", 0.);

    GlobalSettings::instance()->controller()->addLayers(&mLayers, "bb");
    GlobalSettings::instance()->controller()->addLayers(&mRULayers, "bbru");



}

void BarkBeetleModule::setup(const ResourceUnit *ru)
{
    if (!ru)
        return;
    qDebug() << "BB setup for RU" << ru->id();

}

void BarkBeetleModule::run()
{
    // calculate generations
    DebugTimer d("BB:generations");
    ResourceUnit **ruptr = GlobalSettings::instance()->model()->RUgrid().begin();
    BarkBeetleRUCell *bbptr = mRUGrid.begin();
    while (bbptr<mRUGrid.end()) {
        if (*ruptr) {
            bbptr->generations = mGenerations.calculateGenerations(*ruptr);
        }
        ++bbptr; ++ruptr;

    }

    qDebug() << "bark beetle run called";
}

void BarkBeetleModule::yearBegin()
{

}


//*********************************************************************************
//************************************ BarkBeetleLayers ***************************
//*********************************************************************************


double BarkBeetleLayers::value(const BarkBeetleCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.n; // height
    default: throw IException(QString("invalid variable index for a BarkBeetleCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> BarkBeetleLayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("value"), QLatin1Literal("grid value of the pixel"), GridViewRainbow);

}

bool BarkBeetleLayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click
}


double BarkBeetleRULayers::value(const BarkBeetleRUCell &data, const int index) const
{
    switch(index){
    case 0: return data.generations; // number of generation
    default: throw IException(QString("invalid variable index for a BarkBeetleRUCell: %1").arg(index));
    }

}

const QVector<LayeredGridBase::LayerElement> BarkBeetleRULayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("generations"), QLatin1Literal("total number of bark beetle generations"), GridViewHeat);
}

bool BarkBeetleRULayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click

}
