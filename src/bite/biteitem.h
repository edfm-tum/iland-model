#ifndef BITEITEM_H
#define BITEITEM_H

#include "bite_global.h"
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
public:
    explicit BiteItem(QObject *parent = nullptr);
    Q_INVOKABLE BiteItem(QJSValue obj);


    virtual void setup(BiteAgent *agent);

    /// true if the item runs cell by cell
    bool runCells() const { return mRunCells; }

    QString name() const {return mName; }
    void setName(QString name) { mName = name; }


signals:

public slots:
    // actions
    virtual void run();
    virtual void runCell(BiteCell *cell, ABE::FMTreeList *treelist);

protected:
    BiteAgent *agent() const { return mAgent; }
    int cellSize() const;
    virtual QStringList allowedProperties();
    void checkProperties(QJSValue obj);
    QJSValue mObj;
    void setRunCells(bool rc) { mRunCells = rc; }
private:
    BiteAgent *mAgent;
    QString mName;
    QString mDescription;
    bool mRunCells;

};

} // end namespace
#endif // BITEITEM_H
