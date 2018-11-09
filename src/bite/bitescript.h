#ifndef BITESCRIPT_H
#define BITESCRIPT_H
#include <QJSValue>
#include <QObject>
namespace BITE {

class BiteEngine;
class BiteAgent;

class BiteScript: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList agents READ agents)

public:
    BiteScript(QObject *parent=nullptr);
    void setup(BiteEngine *biteengine);

    QStringList agents();

    // static members
    static QString JStoString(QJSValue value);

public slots:
    BiteAgent *agent(QString agent_name);

    void log(QString msg);
    void log(QJSValue obj);

    void run(int year);

private:

    BiteEngine *mEngine;
};


} // end name space
#endif // BITESCRIPT_H
