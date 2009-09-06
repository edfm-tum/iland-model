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

#if !defined(DBG_IF)
#  ifndef QT_NO_DEBUG
#    define DBG_IF(cond, where, what) ((cond) ? dbg_helper(where, what, __FILE__, __LINE__) : qt_noop())
#  else
#    define DBG_IF(cond, where, what) qt_noop()
#  endif
#endif

#if !defined(DBG_IF_X)
#  ifndef QT_NO_DEBUG
#    define DBG_IF_X(cond, where, what,more) ((cond) ? dbg_helper_ext(where, what, __FILE__, __LINE__,more) : qt_noop())
#  else
#    define DBG_IF_X(cond, where, what,more) qt_noop()
#  endif
#endif

#if !defined(DBGMODE)
#  ifndef QT_NO_DEBUG
#    define DBGMODE(stmts) { stmts }
#  else
#    define DBGMODE(stmts) qt_noop()
#  endif
#endif


#endif // GLOBAL_H
