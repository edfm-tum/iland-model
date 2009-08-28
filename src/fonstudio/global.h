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
// general datatypes
//typedef int TreeSpecies;
#endif // GLOBAL_H
