#include <QString>
const char *version = "0.8.6";
const char *svn_revision = "838";
const char *currentVersion(){ return version;}
const char *svnRevision(){ return svn_revision;}

// compiler version
#ifdef Q_CC_MSVC
#define MYCC "MSVC"
#endif
#ifdef Q_CC_GNU
#define MYCC "GCC"
#endif
#ifndef MYCC
#define MYCC "unknown"
#endif

// http://stackoverflow.com/questions/1505582/determining-32-vs-64-bit-in-c
// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef ENVIRONMENT32
#define BITS "32 bit"
#else
#define BITS "64 bit"
#endif

QString compiler()
{
    return QString("%1 %2 Qt %3").arg(MYCC).arg(BITS).arg(qVersion());
}
