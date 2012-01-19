#include "windscript.h"
#include "windmodule.h"

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
