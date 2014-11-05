#ifndef BARKBEETLESCRIPT_H
#define BARKBEETLESCRIPT_H

#include <QObject>
#include <QJSValue>

class BarkBeetleModule; // forward

class BarkBeetleScript : public QObject
{
    Q_OBJECT
    //Q_PROPERTY(JSValue init READ testfun WRITE setTestfun)
public:
    explicit BarkBeetleScript(QObject *parent = 0);
    void setBBModule(BarkBeetleModule *module) { mBeetle = module; }
signals:

public slots:
    void test(QString value);

    void init(QJSValue fun);
    void run(QJSValue fun);
    double pixelValue(int ix, int iy);
    void setPixelValue(int ix, int iy, double val);

private:
    BarkBeetleModule *mBeetle;

};

#endif // BARKBEETLESCRIPT_H
