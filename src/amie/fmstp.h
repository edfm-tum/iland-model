#ifndef FOMESTP_H
#define FOMESTP_H

#include "fmstand.h"
#include <QJSValue>
/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */

class FMSTP
{
public:
    FMSTP();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    /// read the optionfile
    void setup(QJSValue &js_value);

    /// main function that runs the current program for stand 'stand'
    bool execute(FMStand &stand);
private:
    class Schedule {
    public:
        Schedule()  {}
        Schedule(QJSValue &js_value) { clear(); setup(js_value); }
        void clear() { tmin=tmax=topt=-1; tminrel=tmaxrel=toptrel=-1.; force_execution=false; }
        void setup(QJSValue &js_value);
        QString dump() const;
        int tmin; int tmax; int topt;
        double tminrel; double tmaxrel; double toptrel;
        bool force_execution;

    };
    static QJSValue valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage=QString());

};

#endif // FOMESTP_H
