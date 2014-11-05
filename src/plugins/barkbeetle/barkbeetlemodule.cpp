#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"

BarkBeetleModule::BarkBeetleModule()
{
    mLayers.setGrid(mGrid);

}

void BarkBeetleModule::setup()
{
    // setup the wind grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    BarkBeetleCell cell;
    mGrid.initialize(cell);

    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.barkbeetle"));
    qDebug() << "BBSETUP TEST:" << xml.valueDouble(".test", 0.);

    GlobalSettings::instance()->controller()->addLayers(&mLayers, "bb");



}

void BarkBeetleModule::setup(const ResourceUnit *ru)
{
    qDebug() << "BB setup for RU" << ru->id();

}

void BarkBeetleModule::run()
{
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
    default: throw IException(QString("invalid variable index for a WindCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> BarkBeetleLayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("value"), QLatin1Literal("grid value of the pixel"), GridViewRainbow);
}
