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

// log level functions
bool logLevelDebug(); // true, if detailed debug information is logged
bool logLevelInfo(); // true, if only important aggreate info is logged
bool logLevelWarning(); // true if only severe warnings/errors are logged.
void setLogLevel(int loglevel); // setter function
// cool random number generator (using the mersenne-twister) by http://www-personal.umich.edu/~wagnerr/MersenneTwister.html
#include "../3rdparty/MersenneTwister.h"
// access the Mersenne-Twister-Random-Numbers
MTRand &mtRand(); // static object lives in globalsettings
/// nrandom returns a random number from [p1, p2] -> p2 is a possible result!
inline double nrandom(const double& p1, const double& p2)
{
    return p1 + mtRand().rand(p2-p1);
    //return p1 + (p2-p1)*(rand()/double(RAND_MAX));
}
/// returns a random number in [0,1] (i.e.="1" is a possible result!)
inline double drandom()
{
    return mtRand().rand();
    //return rand()/double(RAND_MAX);
}
/// return a random number from "from" to "to" (incl.), i.e. irandom(3,5) results in 3, 4 or 5.
inline int irandom(int from, int to)
{
    return from + mtRand().randInt(to-from);
    //return from +  rand()%(to-from);
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
