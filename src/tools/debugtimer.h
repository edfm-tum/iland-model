#ifndef DEBUGTIMER_H
#define DEBUGTIMER_H
#include "ticktack.h"

/** Timer class that writes timings to the Debug-Output-Channel

The class writes the elapsed time to qDebug() when either destructed, or when explicitely showElapsed() is called.
  elapsed() queries the elapsed time in milliseconds since construction or start() is called. Using interval() one can
  write a message with the time elapsed up the calling time, and the clock is reset afterwards. The name of the timer is
  set during construction. This message is printed when showElapsed() is called or durig destruction.
  Additionally, elapsed times of timers sharing the same caption are aggregated. Use clearAllTimers() to reset and printAllTimers()
  print the sums to the debug console. "Silent" DebugOutputs (setSilent() don't print timings for each iteration, but are still
    counted in the sums. If setAsWarning() is issued, the debug messages are print as warning, thus also visible
  when debug messages are disabled.
  @code void foo() {
     DebugTimer t("foo took [ms]:");
     <some lengthy operation>
 } // will e.g. print "foo took [ms]: 123" to debug console

 void bar() {
    DebugTimer::clearAllTimers(); // set all timers to 0
    for (i=0;i<1000;i++)
       foo();
    DebugTimer::printAllTimers(); // print the sum of the timings.
 }
 @endcode
 For Windows, the "TickTack"-backend is used.

*/
class DebugTimer
{
public:
    DebugTimer() { m_hideShort=false; m_silent=false; start(); }
    DebugTimer(const QString &caption, bool silent=false);
    void setSilent() { m_silent=true; }
    void setHideShort(bool hide_short_messages) { m_hideShort = hide_short_messages; }
    ~DebugTimer();
    void showElapsed();
    double elapsed(); // elapsed time in milliseconds
    void start();
    void interval(const QString &text);
    static void clearAllTimers();
    static void printAllTimers();
private:
    static QHash<QString, double> mTimingList;
    TickTack t;
    bool m_hideShort; // if true, hide messages for short operations (except an explicit call to showElapsed())
    bool m_shown;
    bool m_silent;
    QString m_caption;
};

#endif // DEBUGTIMER_H
