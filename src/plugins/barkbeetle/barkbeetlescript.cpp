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

#include "barkbeetlescript.h"

#include "barkbeetlemodule.h"
#include "outputmanager.h"


BarkBeetleScript::BarkBeetleScript(QObject *)
{
    mBeetle = 0;
}

void BarkBeetleScript::test(QString value)
{
    qDebug() << value;
}

void BarkBeetleScript::init(QJSValue fun)
{
    QJSValueList args = QJSValueList() << 0 << 0;

    if (!fun.isCallable()) {
        qDebug() << "no valid function in init!!";
        return;
    }
    for (int y=0;y < mBeetle->mGrid.sizeY(); ++y)
        for (int x=0;x < mBeetle->mGrid.sizeX(); ++x) {
            args[0] = x; args[1] = y;
            double result = fun.call(args).toNumber();
            mBeetle->mGrid.valueAtIndex(x,y).n = result;
        }
}

void BarkBeetleScript::run(QJSValue fun)
{
    QJSValueList args = QJSValueList() << 0 << 0;

    if (!fun.isCallable()) {
        qDebug() << "no valid function in run!!";
        return;
    }
    for (int y=0;y < mBeetle->mGrid.sizeY(); ++y)
        for (int x=0;x < mBeetle->mGrid.sizeX(); ++x) {
            args[0] = x; args[1] = y;
            fun.call(args);
        }
}

double BarkBeetleScript::pixelValue(int ix, int iy)
{
    if (mBeetle->mGrid.isIndexValid(ix,iy)) {
        return mBeetle->mGrid.valueAtIndex(ix, iy).n;
    } else {
        return -9999;
    }
}

void BarkBeetleScript::setPixelValue(int ix, int iy, double val)
{
    if (mBeetle->mGrid.isIndexValid(ix,iy)) {
        mBeetle->mGrid.valueAtIndex(ix, iy).n = val;
    }
}

double BarkBeetleScript::generations(int ix, int iy)
{
    if (mBeetle->mGrid.isIndexValid(ix,iy)) {
        return mBeetle->mRUGrid.valueAt( mBeetle->mGrid.cellCenterPoint(QPoint(ix,iy)) ).generations;
    } else {
        return -9999;
    }

}

void BarkBeetleScript::reloadSettings()
{
    mBeetle->loadParameters();
    mBeetle->loadAllVegetation();
}

void BarkBeetleScript::runBB(int iteration)
{
    qDebug() << "running bark beetle module....";
    mBeetle->run(iteration);
    GlobalSettings::instance()->outputManager()->save(); // just make sure database outputs are properly written
}

void BarkBeetleScript::clear()
{
    qDebug() << "clear bark beetle module....";
    mBeetle->clearGrids();
}

bool BarkBeetleScript::simulate()
{
    return mBeetle->simulate();
}

void BarkBeetleScript::setSimulate(bool do_simulate)
{
    mBeetle->setSimulate(do_simulate);
}
