#ifndef FOMESTP_H
#define FOMESTP_H

#include "fmstand.h"
#include <QJSValue>
/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */
class Expression; // forward

class FMSTP
{
public:
    FMSTP();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    /// read the options from a javascript structure / object
    void setup(QJSValue &js_value);
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode


    /// main function that runs the current program for stand 'stand'
    bool execute(FMStand &stand);
private:
    class Schedule {
    public:
        // setup and life cycle
        Schedule()  {}
        Schedule(QJSValue &js_value) { clear(); setup(js_value); }
        void clear() { tmin=tmax=topt=-1; tminrel=tmaxrel=toptrel=-1.; force_execution=false; }
        void setup(QJSValue &js_value);
        QString dump() const;
        // functions
        double value(const FMStand *stand);
        // some stuffs
        int tmin; int tmax; int topt;
        double tminrel; double tmaxrel; double toptrel;
        bool force_execution;
    };
    class Events {
    public:
        Events() {}
        /// setup events from the javascript object
        void setup(QJSValue &js_value, QStringList event_names);
        QString run(const QString event, const FMStand *stand); ///< execute javascript event if registered
        QString dump(); ///< prints some info
    private:
        QMap<QString, QJSValue> mEvents;
    };
    class Constraints {
    public:
        Constraints() {}
        void setup(QJSValue &js_value); ///< setup from javascript
        bool evaluate(const FMStand *stand); ///< run the constraints
    private:
        struct constraint_item {
            constraint_item(): filter_type(ftInvalid), expr(0) {}
            ~constraint_item();
            void setup(QJSValue &js_value);
            bool evaluate(const FMStand *stand) const;

            enum { ftInvalid, ftExpression, ftJavascript} filter_type;
            Expression *expr;
            QJSValue func;
        };

        QList<constraint_item> mConstraints;
    };

    static QJSValue valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage=QString());
    static bool mVerbose; ///< debug mode

};

#endif // FOMESTP_H
