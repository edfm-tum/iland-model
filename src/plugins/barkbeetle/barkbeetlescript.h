#ifndef BARKBEETLESCRIPT_H
#define BARKBEETLESCRIPT_H

#include <QObject>
#include <QJSValue>

class BarkBeetleModule; // forward

class BarkBeetleScript : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue onClick READ onClick WRITE setOnClick)
public:
    explicit BarkBeetleScript(QObject *parent = 0);
    void setBBModule(BarkBeetleModule *module) { mBeetle = module; }
    QJSValue onClick() const { return mOnClick; }
    void setOnClick(QJSValue handler) { mOnClick = handler; }
signals:

public slots:
    void test(QString value);

    void init(QJSValue fun);
    void run(QJSValue fun);
    double pixelValue(int ix, int iy);
    void setPixelValue(int ix, int iy, double val);
    double generations(int ix, int iy);

    // the real thing
    void runBB();

private:
    QJSValue mOnClick;
    BarkBeetleModule *mBeetle;

};

#endif // BARKBEETLESCRIPT_H
