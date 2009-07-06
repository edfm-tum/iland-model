#ifndef GLOBAL_H
#define GLOBAL_H

#define MSGRETURN(x) { qDebug() << x; return; }
// conversions rad/degree
#define RAD(x) (x*M_PI/180.)
#define GRAD(x) (x/M_PI*180.)
#define PI2 2*M_PI
#endif // GLOBAL_H
