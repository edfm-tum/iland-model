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
// fake implementation
class TTickTack
{
public:
    TTickTack() {}
    void reset() {}
    double elapsed() { return 1.; }
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
