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
    lightResponse = QSharedPointer<Expression>(new Expression(xml.value("lightResponse", "lri")));
    epsilon = xml.valueDouble("epsilon",1.8); // max light use efficiency (aka alpha_c)
    airDensity = xml.valueDouble("airDensity", 1.2);
    heatCapacityAir = xml.valueDouble("heatCapacityAir", 1012);

    XmlHelper site(GlobalSettings::instance()->settings().node("model.site"));
    latitude = RAD(site.valueDouble("latitude",48.));
    nitrogenAvailable = site.valueDouble("availableNitrogen", 40);
    waterholdingCapacity = site.valueDouble("waterholdingCapacity", 100);

}

void ModelSettings::print()
{
    QStringList set=QStringList() << "Settings:";
    set << QString("growthEnabled=%1").arg(growthEnabled);
    set << QString("mortalityEnabled=%1").arg(mortalityEnabled);
    set << QString("lightExtinctionCoefficient=%1").arg(lightExtinctionCoefficient);
    set << QString("lightExtinctionCoefficientOpacity=%1").arg(lightExtinctionCoefficientOpacity);
    set << QString("temperatureTau=%1").arg(temperatureTau);
    set << QString("lightResponse=%1").arg(lightResponse->expression());
    set << QString("epsilon=%1").arg(epsilon);
    set << QString("airDensity=%1").arg(airDensity);
    set << QString("heatCapacityAir=%1").arg(heatCapacityAir);

    set << QString("latitude=%1").arg(GRAD(latitude));
    set << QString("availableNitrogen=%1").arg(nitrogenAvailable);
    set << QString("waterholdingCapacity=%1").arg(waterholdingCapacity);

    qDebug() << set.join("\n");
}
