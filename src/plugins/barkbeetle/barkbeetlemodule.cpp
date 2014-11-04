#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"

BarkBeetleModule::BarkBeetleModule()
{
}

void BarkBeetleModule::setup()
{
    // setup the wind grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());

    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.barkbeetle"));
    qDebug() << "BBSETUP TEST:" << xml.valueDouble(".test", 0.);


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
