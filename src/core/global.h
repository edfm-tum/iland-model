#ifndef GLOBAL_H
#define GLOBAL_H

#define MSGRETURN(x) { qDebug() << x; return; }
#define WARNINGRETURN(x) { qWarning() << x; return; }
#define ERRORRETURN(x) { qError() << x; return; }
// conversions rad/degree
#define RAD(x) (x*M_PI/180.)
#define GRAD(x) (x/M_PI*180.)
#define PI2 2*M_PI


#include "globalsettings.h"
#include "exception.h"
// general datatypes
//typedef int TreeSpecies;

// global debug helpers (used by macros!)
void dbg_helper(const char *where, const char *what,const char* file,int line);
void dbg_helper_ext(const char *where, const char *what,const char* file,int line, const QString &s);

// change to enabled detailed debug messages.
// if NO_DEBUG_MSGS is defined, NO debug outputs are generated.
#if defined(QT_NO_DEBUG)
#define NO_DEBUG_MSGS
#endif

#if !defined(DBG_IF)
#  ifndef NO_DEBUG_MSGS
#    define DBG_IF(cond, where, what) ((cond) ? dbg_helper(where, what, __FILE__, __LINE__) : qt_noop())
#  else
#    define DBG_IF(cond, where, what) qt_noop()
#  endif
#endif

#if !defined(DBG_IF_X)
#  ifndef NO_DEBUG_MSGS
#    define DBG_IF_X(cond, where, what,more) ((cond) ? dbg_helper_ext(where, what, __FILE__, __LINE__,more) : qt_noop())
#  else
#    define DBG_IF_X(cond, where, what,more) qt_noop()
#  endif
#endif

#if !defined(DBGMODE)
#  ifndef NO_DEBUG_MSGS
#    define DBGMODE(stmts) { stmts }
#  else
#    define DBGMODE(stmts) qt_noop()
#  endif
#endif

/// nrandom returns a random number from [p1, p2]
inline double nrandom(const double& p1, const double& p2)
{
    return p1 + (p2-p1)*(rand()/double(RAND_MAX));
}
/// returns a random number in [0,1]
inline double drandom()
{
    return rand()/double(RAND_MAX);
}
inline int irandom(int from, int to)
{
    return from +  rand()%(to-from);
}

inline double limit(const double value, const double lower, const double upper)
{
    return qMax(qMin(value, upper), lower);
}
inline int limit(const int value, const int lower, const int upper)
{
    return qMax(qMin(value, upper), lower);
}
inline void setBit(int &rTarget, const int bit, const bool value)
{
    if (value)
        rTarget |= (1 << bit);  // set bit
    else
        rTarget &= ( (1 << bit) ^ 0xffffff ); // clear bit
}
inline bool isBitSet(const int value, const int bit)
{
    return value & (1 << bit);
}
#endif // GLOBAL_H
