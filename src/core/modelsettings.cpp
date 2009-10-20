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
    airPressure = xml.valueDouble("airPressure", 1013);
    heatCapacityAir = xml.valueDouble("heatCapacityAir", 1012);


    XmlHelper world(GlobalSettings::instance()->settings().node("model.world"));
    latitude = RAD(world.valueDouble("latitude",48.));
}

void ModelSettings::print()
{
    QStringList set=QStringList() << "Settings:";
    set << QString("growthEnabled=%1").arg(growthEnabled);
    set << QString("mortalityEnabled=%1").arg(mortalityEnabled);
    set << QString("lightExtinctionCoefficient=%1").arg(lightExtinctionCoefficient);
    set << QString("lightExtinctionCoefficientOpacity=%1").arg(lightExtinctionCoefficientOpacity);
    set << QString("temperatureTau=%1").arg(temperatureTau);
    set << QString("epsilon=%1").arg(epsilon);
    set << QString("airDensity=%1").arg(airDensity);
    set << QString("airPressure=%1").arg(airPressure);
    set << QString("heatCapacityAir=%1").arg(heatCapacityAir);

    set << QString("latitude=%1").arg(GRAD(latitude));

    qDebug() << set.join("\n");
}
