/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

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
    try {
    mModule->run(iteration, true);
    } catch (const IException &e) {
       qDebug() << "ERROR in windEvent():" << e.message();
    }

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

void WindScript::initialize()
{
    mModule->setup();
    qDebug() << "initialized the wind module.";
}
