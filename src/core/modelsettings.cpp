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

    XmlHelper site(GlobalSettings::instance()->settings().node("model.world.site"));
    latitude = RAD(xml.valueDouble("latitude",48.));
    nitrogenAvailable = xml.valueDouble("nitrogenAvailable", 40);
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
    set << QString("latitude=%1").arg(GRAD(latitude));
    set << QString("nitrogenAvailable=%1").arg(nitrogenAvailable);

    qDebug() << set.join("\n");
}
