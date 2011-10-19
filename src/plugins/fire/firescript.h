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
#ifndef FIRESCRIPT_H
#define FIRESCRIPT_H

#include <QObject>
class FireModule; // forward
class FireScript : public QObject
{
    Q_OBJECT
public:
    explicit FireScript(QObject *parent = 0);
    void setFireModule(FireModule *module) { mFire = module; }

signals:

public slots:
/**    Ignite a fire event with pre-defined properties, i.e. at a given location (parameters x_meter, y_meter) with given conditions (i.e. wind direction and speed). The fire size (as taken from the distribution)
    x_meter, y_meter: metric coordinates of the ignition point in the landscape
    firesize: provide a fire size (m2). The fire size will be drawn from the fire size distribution if firesize=-1 or omitted.
    windspeed: wind speed (m/s), drawn randomly if omitted or -1.
    winddirection: wind direction (0°=N..180°=S..270=W°), drawn randomly if omitted or set to -1.
    Returns the burnt area */
    double ignite(double x, double y, double firesize=-1, double windspeed=-1, double winddirection=-1);
private:
    FireModule *mFire;
};

#endif // FIRESCRIPT_H
