#include "global.h"
#include "modelsettings.h"
#include "expression.h"

ModelSettings::ModelSettings()
{
}

void ModelSettings::loadModelSettings()
{
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.settings"));
    growthEnabled = xml.valueBool("growthEnabled", true);
    mortalityEnabled = xml.valueBool("mortalityEnabled", true);
    lightExtinctionCoefficient = xml.valueDouble("lightExtinctionCoefficient", 0.5);
    lightExtinctionCoefficientOpacity=xml.valueDouble("lightExtinctionCoefficientOpacity", 0.5);
    temperatureTau=xml.valueDouble("temperatureTau",5);
    epsilon = xml.valueDouble("epsilon",1.8); // max light use efficiency (aka alpha_c)
    airDensity = xml.valueDouble("airDensity", 1.2);
    laiThresholdForClosedStands = xml.valueDouble("laiThresholdForClosedStands", 3.);
    boundaryLayerConductance = xml.valueDouble("boundaryLayerConductance", 0.2);
    XmlHelper world(GlobalSettings::instance()->settings().node("model.world"));
    latitude = RAD(world.valueDouble("latitude",48.));
    usePARFractionBelowGroundAllocation = xml.valueBool("usePARFractionBelowGroundAllocation", true);
    //useDynamicAvailableNitrogen = xml.valueBool("model.settings.soil.useDynamicAvailableNitrogen", false); // TODO: there is a bug in using a xml helper that whose top-node is set
    useDynamicAvailableNitrogen = GlobalSettings::instance()->settings().valueBool("model.settings.soil.useDynamicAvailableNitrogen", false);
    topLayerWaterContent = xml.valueDouble("topLayerWaterContent",50);
}

void ModelSettings::print()
{
    if (!logLevelInfo()) return;
    QStringList set=QStringList() << "Settings:";
    set << QString("growthEnabled=%1").arg(growthEnabled);
    set << QString("mortalityEnabled=%1").arg(mortalityEnabled);
    set << QString("lightExtinctionCoefficient=%1").arg(lightExtinctionCoefficient);
    set << QString("lightExtinctionCoefficientOpacity=%1").arg(lightExtinctionCoefficientOpacity);
    set << QString("temperatureTau=%1").arg(temperatureTau);
    set << QString("epsilon=%1").arg(epsilon);
    set << QString("airDensity=%1").arg(airDensity);
    set << QString("useDynamicAvailableNitrogen=%1").arg(useDynamicAvailableNitrogen);
    set << QString("topLayerWaterContent=%1").arg(topLayerWaterContent);

    set << QString("latitude=%1").arg(GRAD(latitude));

    qDebug() << set.join("\n");
}
