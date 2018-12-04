#ifndef BITEAGENT_H
#define BITEAGENT_H

#include <QObject>
#include <QJSValue>

#include "bitecell.h"
#include "grid.h"
#include "biteitem.h"
#include "bitecellscript.h"
#include "bitewrapper.h"
#include "biteclimate.h"

namespace ABE {
class FMTreeList; // forward
}
class ScriptGrid;

namespace BITE {

struct BAgentStats {
    BAgentStats() { clear(); }
    void clear() { nDispersal=nColonizable=nActive=nNewlyColonized=treesKilled = 0; agentBiomass=m3Killed=totalImpact=0.; }
    int nDispersal; ///< number of cells that are active source of dispersal
    int nColonizable; ///< number of cells that are tested for colonization
    int nActive; ///< number of cells that are active at the end of the year
    int nNewlyColonized; ///< number of cells that are colonized (successfully)
    double agentBiomass; ///< total agent biomass in all active cells
    int treesKilled; ///< number of all killed trees
    double m3Killed; ///< volume of all killed trees
    double totalImpact; /// impact on tree compartments (depending on the mode)
};

class BiteLifeCycle;

class BiteAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(int cellSize READ cellSize)
    Q_PROPERTY(int width READ width)
    Q_PROPERTY(int height READ height)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose)
    Q_PROPERTY(ScriptGrid* drawGrid READ drawGrid)
    Q_PROPERTY(QStringList variables READ variables)

public:
    BiteAgent(QObject *parent = nullptr);
    Q_INVOKABLE BiteAgent(QJSValue obj);
    ~BiteAgent();

    ScriptGrid *drawGrid() { Q_ASSERT(mDrawGrid != nullptr); return mDrawGrid; }
    Grid<double> *baseDrawGrid() { return &mBaseDrawGrid; }


    /// setup of the agent with a data structure (provided via JS)
    void setup(QJSValue obj);
    /// helper function to set C++ ownership for 'obj'
    static void setCPPOwnership(QObject *obj);
    BiteWrapperCore *wrapper() { return &mWrapperCore; }
    const BiteClimate &biteClimate() const { return mClimateProvider; }

    void notifyItems(BiteCell *cell, BiteCell::ENotification what);


    /// (short) name of the agent
    QString name() const {return mName; }
    /// (user defined) description of the agent
    QString description() const {return mDesc; }
    /// cell size in meters
    int cellSize() const { return mCellSize; }
    int width() const {return grid().sizeX(); }
    int height() const {return grid().sizeY(); }

    // return a list of all currently available cell variables
    QStringList variables();

    bool verbose() const { return mVerbose; }
    void setVerbose(bool v) { mVerbose = v; }

    const Grid<BiteCell*> &grid() const { return mGrid; }
    static ABE::FMTreeList* threadTreeList();
    BAgentStats &stats()  { return mStats; }
    BiteLifeCycle *lifeCycle() const { return mLC; }


public slots:
    BiteCellScript *cell(int x, int y);
    /// returns true, if a valid cell is at x/y
    bool isCellValid(int x, int y) { return grid().isIndexValid(x,y); }
    void run();
    void run(BiteCellScript *cell);
    /// elements of the agent
    BiteItem* item(QString name);
    QString info();

    double evaluate(BiteCellScript *cell, QString expr);
    void addVariable(ScriptGrid *grid, QString var_name);
    void updateDrawGrid(QString expression);
    void saveGrid(QString expression, QString file_name);
private:
    static void runCell(BiteCell &cell);
    static QHash<QThread*, ABE::FMTreeList* > mTreeLists;

    BiteWrapperCore mWrapperCore;
    BiteClimate mClimateProvider;

    BAgentStats mStats;

    // grids
    void createBaseGrid();
    Grid<BiteCell*> mGrid;
    QVector<BiteCell> mCells;

    // grid for drawing
    Grid<double> mBaseDrawGrid;
    ScriptGrid *mDrawGrid;

    Events mEvents;
    QJSValue mThis;

    // elements (i.e. processes)
    QVector<BiteItem*> mItems;

    BiteLifeCycle *mLC;

    QString mName;
    QString mDesc;
    int mCellSize;
    bool mVerbose;

};

} // end namespace

#endif // BITEAGENT_H
