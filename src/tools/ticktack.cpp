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

#include "ticktack.h"
#include <QtCore>

/* TODO: this is purely WINDOWS - provide a version for other BS, change the build-system
   to only use this on Win.
*/
#ifdef Q_WS_WIN
#include <windows.h>

class TTickTack
{
  private:
    _LARGE_INTEGER starttime;

_LARGE_INTEGER getCount()
{
  _LARGE_INTEGER value;
  QueryPerformanceCounter(&value);
  return  value;
}
  public:
    TTickTack() { reset(); }
    void reset() { starttime = getCount(); }
    double elapsed() {
        _LARGE_INTEGER stoptime = getCount();
        __int64 start = starttime.QuadPart;
        __int64 end = stoptime.QuadPart;
        __int64 elapsed = end - start;
        _LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        double seconds = elapsed / double(freq.QuadPart);
        return seconds;

    }
};
#else
// LINUX implementation using standard Qt timer
#include <QTime>
class TTickTack
{
public:
    TTickTack() { qt_timer.start();}
    void reset() { qt_timer.start(); }
    double elapsed() { return qt_timer.elapsed()/1000.; // return seconds }
private:
    QTime qt_timer;
};
#endif

TickTack::TickTack()
{
    d = new TTickTack();
    d->reset();
}
TickTack::~TickTack()
{
    delete d;
}

void TickTack::start()
{
    d->reset();
}

double TickTack::elapsed()
{
    return d->elapsed();
}
