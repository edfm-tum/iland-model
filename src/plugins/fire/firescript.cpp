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
#include "firescript.h"
#include "firemodule.h"
#include "helper.h"

FireScript::FireScript(QObject *parent) :
    QObject(parent)
{
    mFire = 0;
}

double FireScript::ignite(double x, double y, double firesize, double windspeed, double winddirection)
{
    qDebug() << "Fireevent triggered by javascript: " << x << y << firesize << windspeed << winddirection;
    mFire->prescribedIgnition(x, y, firesize, windspeed, winddirection);
    return 0.;
}

QString fireRUValueType="";
QString fireRUValue(const FireRUData &data) {
    if (fireRUValueType=="kbdi") return QString::number(data.kbdi());
    if (fireRUValueType=="basalarea") return QString::number(data.fireRUStats.basal_area>0?data.fireRUStats.died_basal_area / data.fireRUStats.basal_area:0.);
    return "Error";
}

bool FireScript::gridToFile(QString grid_type, QString file_name)
{
    if (!GlobalSettings::instance()->model())
        return false;
    QString result;
    if (grid_type == "spread") {
        result = gridToESRIRaster(mFire->mGrid);
    } else {
        fireRUValueType = grid_type;
        result = gridToESRIRaster(mFire->mRUGrid, &fireRUValue); // use a specific value function (see above)
    }

    if (!result.isEmpty()) {
        file_name = GlobalSettings::instance()->path(file_name);
        Helper::saveToTextFile(file_name, result);
        qDebug() << "saved grid to " << file_name;
        return true;
    }
    qDebug() << "could not save gridToFile because" << grid_type << "is not a valid grid.";
    return false;

}