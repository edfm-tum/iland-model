#ifndef BITEENGINE_H
#define BITEENGINE_H

#include "biteagent.h"
#include "bitescript.h"


namespace BITE {

class BiteEngine
{
public:
    /// get singleton of the BiteEngine
    static BiteEngine *instance() {if (mInstance) return mInstance;  mInstance=new BiteEngine(); return mInstance; }
    ~BiteEngine();

    void setup();
    void addAgent(BiteAgent *new_agent);
    /// get agent by name
    BiteAgent *agentByName(QString name);
    QStringList agentNames();

    // properties
    QJSEngine *scriptEngine();

    // the current simulation year
    int currentYear() const {return mYear; }
    void setYear(int year) {mYear = year; }

    Grid<double>* preparePaintGrid(QObject *handler, QString name);

    // functions
    void run();

    /// called from agents/items if an error occured during script execution
    void error(QString error_msg);

    /// safe guard calls to the JS engine (only 1 thread allowed)
    QMutex *serializeJS() { return &mSerialize; }

    // static functions
    static QJSValue valueFromJs(const QJSValue &js_value, const QString &key, const QString default_value=QLatin1Literal(""), const QString &errorMessage=QLatin1Literal(""));
private:
    void resetErrors();
    BiteEngine(); // private ctor
    static BiteEngine *mInstance;
    QList<BiteAgent*> mAgents;
    BiteScript mScript;
    QStringList mErrorStack;
    bool mHasScriptError;
    QMutex mSerialize;
    int mYear;
    bool mRunning;
};

} // end namespaec
#endif // BITEENGINE_H
