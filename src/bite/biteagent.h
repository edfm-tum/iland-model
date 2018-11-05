#ifndef BITEAGENT_H
#define BITEAGENT_H

#include <QObject>
#include <QJSValue>

#include "bitecell.h"
#include "grid.h"
#include "biteitem.h"
namespace ABE {
class FMTreeList; // forward
}

namespace BITE {

class BiteAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(int cellSize READ cellSize)
public:
    BiteAgent(QObject *parent = nullptr);
    Q_INVOKABLE BiteAgent(QJSValue obj);

    /// setup of the agent with a data structure (provided via JS)
    void setup(QJSValue obj);

    /// (short) name of the agent
    QString name() const {return mName; }
    /// (user defined) description of the agent
    QString description() const {return mDesc; }
    /// cell size in meters
    int cellSize() const { return mCellSize; }

    const Grid<BiteCell*> &grid() const { return mGrid; }
    static ABE::FMTreeList* threadTreeList();

public slots:
    void run();
    BiteItem* item(QString name);
private:
    static void runCell(BiteCell &cell);
    static QHash<QThread*, ABE::FMTreeList* > mTreeLists;

    // grid
    void createBaseGrid();
    Grid<BiteCell*> mGrid;
    QVector<BiteCell> mCells;

    // elements (i.e. processes)
    QVector<BiteItem*> mItems;

    QString mName;
    QString mDesc;
    int mCellSize;

};

} // end namespace

#endif // BITEAGENT_H
