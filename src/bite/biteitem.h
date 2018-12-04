#ifndef BITEITEM_H
#define BITEITEM_H

#include "bite_global.h"
#include "bitecell.h"
#include <QObject>

namespace ABE {
class FMTreeList; // forward
}

namespace BITE {

class BiteAgent;
class BiteCell;

class BiteItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(BiteAgent* agent READ agent)
public:
    explicit BiteItem(QObject *parent = nullptr);
    Q_INVOKABLE BiteItem(QJSValue obj);
    BiteAgent *agent() const { return mAgent; }


    virtual void setup(BiteAgent *agent);
    virtual QString info();

    /// true if the item runs cell by cell
    bool runCells() const { return mRunCells; }

    QString name() const {return mName; }
    void setName(QString name) { mName = name; }

    QString description() const {return mDescription; }


    virtual void notify(BiteCell *cell, BiteCell::ENotification what);
signals:

public slots:
    // actions
    virtual void afterSetup(); ///< called once after the full agent is set up
    virtual void beforeRun(); ///< for initializations, called every year
    virtual void run();
    virtual void runCell(BiteCell *cell, ABE::FMTreeList *treelist);

protected:

    int cellSize() const;
    virtual QStringList allowedProperties();
    void checkProperties(QJSValue obj);
    QJSValue thisJSObj() { return mThis; }
    QJSValue mObj; ///< the JS value used to setup the item
    QJSValue mThis; ///< the JS representation of the item
    void setRunCells(bool rc) { mRunCells = rc; }
    bool verbose();
private:
    BiteAgent *mAgent;
    QString mName;
    QString mDescription;
    bool mRunCells;

};

} // end namespace
#endif // BITEITEM_H
