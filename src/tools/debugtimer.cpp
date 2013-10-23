#include "global.h"
#include "debugtimer.h"

// static members
QHash<QString, double> DebugTimer::mTimingList;

/*
double DebugTimer::m_tick_p_s=0.;

void DebugTimer::sampleClock(int ms)
{
    QTime t;
    t.start();
    ticks now = getticks();

    while (t.elapsed() < ms) {
       int t;
       double x=0.;
       for (t=0; t<100; t++)
           x+=sin(x);
    }
    int el = t.elapsed();
    double tickselapsed = elapsed(getticks(), now);
    m_tick_p_s = tickselapsed / double(el);
    qDebug() << ms << "ms -> ticks/msec" << m_tick_p_s << "ticks elapsed" << tickselapsed;

}*/
DebugTimer::~DebugTimer()
{
    double t = elapsed();
    mTimingList[m_caption]+=t;
    // show message if timer is not set to silent, and if time > 100ms (if timer is set to hideShort (which is the default))
    if (!m_silent && (!m_hideShort || t>100.))
        showElapsed();
}

QMutex timer_mutex;
DebugTimer::DebugTimer(const QString &caption, bool silent)
{
    m_caption = caption;
    m_silent=silent;
    m_hideShort=true;
    if (!mTimingList.contains(caption)) {
        QMutexLocker locker(&timer_mutex);
        if (!mTimingList.contains(caption))
            mTimingList[caption]=0.;
    }
    start();
}

void DebugTimer::clearAllTimers()
{
    QHash<QString, double>::iterator i = mTimingList.begin();
     while (i != mTimingList.end()) {
         i.value() = 0.;
         ++i;
     }
}
void DebugTimer::printAllTimers()
{
    QHash<QString, double>::iterator i = mTimingList.begin();
    qWarning() << "Total timers\n================";
    double total=0.;
    while (i != mTimingList.end()) {
         if (i.value()>0)
            qWarning() << i.key() << ":" << i.value() << "ms";
         total+=i.value();
         ++i;
     }
    qWarning() << "Sum: " << total << "ms";
}

void DebugTimer::interval(const QString &text)
{
    double elapsed_time = elapsed();
    qDebug() << "Timer" << text << elapsed_time << "ms";
    start();
}

void DebugTimer::showElapsed()
{
    if (!m_shown) {
            qDebug() << "Timer" << m_caption << ":" << elapsed() << "ms";
    }
    m_shown=true;
}
double DebugTimer::elapsed()
{
    return t.elapsed()*1000;
}

void DebugTimer::start()
{
    t.start();
    m_shown=false;
}
