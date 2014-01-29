#ifndef FOMESTP_H
#define FOMESTP_H

#include "fmstand.h"
#include <QJSValue>
#include <QList>

class Expression; // forward

namespace AMIE {

/**
 * @brief The FMSTP class encapsulates one "stand treatment program", that consists of several "activities".
 */
class Activity; // forward

class FMSTP
{
public:
    FMSTP();
    ~FMSTP();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    /// read the options from a javascript structure / object
    void setup(QJSValue &js_value, const QString name=QString(), int level=0);
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode
    static QJSValue valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value, const QString &errorMessage=QString());


    /// main function that runs the current program for stand 'stand'
    bool execute(FMStand &stand);

    // helper functions
    void dumpInfo();

private:
    QString mName; ///< the name of the stand treatment program
    void setupActivity(QJSValue &js_value);
    void clear(); ///< remove all activites
    static bool mVerbose; ///< debug mode
    QList<Activity*> mActivities; ///< container for the activities of the STP

};

} // namespace

#endif // FOMESTP_H
