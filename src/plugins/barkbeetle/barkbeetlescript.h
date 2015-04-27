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

#ifndef BARKBEETLESCRIPT_H
#define BARKBEETLESCRIPT_H

#include <QObject>
#include <QJSValue>

class BarkBeetleModule; // forward

class BarkBeetleScript : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue onClick READ onClick WRITE setOnClick)
    Q_PROPERTY(bool simulate READ simulate WRITE setSimulate)

public:
    explicit BarkBeetleScript(QObject *parent = 0);
    void setBBModule(BarkBeetleModule *module) { mBeetle = module; }
    QJSValue onClick() const { return mOnClick; }
    void setOnClick(QJSValue handler) { mOnClick = handler; }
    // properties
    bool simulate();
    void setSimulate(bool do_simulate);
signals:

public slots:
    void test(QString value);

    void init(QJSValue fun);
    void run(QJSValue fun);
    double pixelValue(int ix, int iy);
    void setPixelValue(int ix, int iy, double val);

    /// access the number of bark beetle generation at position ix/iy (indices on the 10m grid)
    double generations(int ix, int iy);

    // the real thing
    void reloadSettings(); ///< reload the BB-Module settings from the XML-File
    void runBB(int iteration); ///< run a full cycle of the bark beetle module
    void clear(); ///< reset the barkbeetle module (clear damage and spread data - makes only sense if in simulation mode)




private:

    QJSValue mOnClick;
    BarkBeetleModule *mBeetle;

};

#endif // BARKBEETLESCRIPT_H
