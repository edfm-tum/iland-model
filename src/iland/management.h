#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <QObject>


class QScriptEngine;
class Management : public QObject
{
    Q_OBJECT
public:
    Management();
    ~Management();
    void run();
    void loadScript(const QString &fileName);
public slots:
    void remain(int number);
    void kill(int number);
private:
    QScriptEngine *mEngine;
    int mRemoved;
};

#endif // MANAGEMENT_H
