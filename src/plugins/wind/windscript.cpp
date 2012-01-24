#include "windscript.h"
#include "windmodule.h"
#include "helper.h"

WindScript::WindScript(QObject *parent) :
    QObject(parent)
{
    mModule = 0;
}

int WindScript::windEvent(double windspeed, double winddirection, int max_iteration, bool simulate, int iteration)
{
    mModule->setWindProperties(winddirection*M_PI/180., windspeed);
    mModule->setSimulationMode(simulate);
    mModule->setMaximumIterations(max_iteration);
    mModule->run(iteration);
    qDebug() << "run wind module from script...";
    return 0;
}

bool WindScript::gridToFile(QString grid_type, QString file_name)
{
    if (!GlobalSettings::instance()->model())
        return false;
    QString result;

    result = gridToESRIRaster(mModule->mWindLayers, grid_type); // use a specific value function (see above)

    if (!result.isEmpty()) {
        file_name = GlobalSettings::instance()->path(file_name);
        Helper::saveToTextFile(file_name, result);
        qDebug() << "saved grid to " << file_name;
        return true;
    }
    qDebug() << "could not save gridToFile because" << grid_type << "is not a valid grid.";
    return false;

}
