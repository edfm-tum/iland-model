/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

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
    void clear() { nDispersal=nColonizable=nActive=nNewlyColonized=treesKilled = 0; agentBiomass=m3Killed=totalImpact=0.; saplingsKilled=saplingsImpact=0; }
    int nDispersal; ///< number of cells that are active source of dispersal
    int nColonizable; ///< number of cells that are tested for colonization
    int nActive; ///< number of cells that are active at the end of the year
    int nNewlyColonized; ///< number of cells that are colonized (successfully)
    double agentBiomass; ///< total agent biomass in all active cells
    int treesKilled; ///< number of all killed trees (>4m)
    double m3Killed; ///< volume of all killed trees (>4m)
    double totalImpact; ///< impact on tree compartments (depending on the mode)
    int saplingsKilled; ///< number of saplings (cohorts) killed (<4m)
    int saplingsImpact; ///< number of saplings affected (e.g. by browsing) (<4m)
};

struct BACellStat {
    BACellStat() { clear(); }
    void clear() { nKilled=0; nHostTrees=0; m3Killed = 0.; totalImpact = 0.; saplingsKilled=0; saplingsImpact=0; }
    int nKilled; ///< number of trees (>4m) killed
    int nHostTrees; ///< number of trees (>4m) that are potential host trees (hostFilter)
    double m3Killed; ///< volume of all killed trees (>4m)
    double totalImpact; ///< impact on tree compartments (depending on the mode)
    int saplingsKilled; ///< number of saplings (cohorts) killed (<4m)
    int saplingsImpact; ///< number of saplings affected (e.g. by browsing) (<4m)

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
    static ABE::FMSaplingList* threadSaplingList();
    BAgentStats &stats()  { return mStats; }
    BiteLifeCycle *lifeCycle() const { return mLC; }
    /// create stats grid (on demand)
    void createStatsGrid();
    /// return the cell statistics (if available) for the given 'cell'
    BACellStat *cellStat(const BiteCell *cell);


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
    void updateDrawGrid(QJSValue func);
    void saveGrid(QString expression, QString file_name);
private:
    static void runCell(BiteCell &cell);
    static QHash<QThread*, ABE::FMTreeList* > mTreeLists;
    static QHash<QThread*, ABE::FMSaplingList* > mSaplingLists;

    BiteWrapperCore mWrapperCore;
    BiteClimate mClimateProvider;

    BAgentStats mStats;

    // grids
    void createBaseGrid();
    Grid<BiteCell*> mGrid;
    QVector<BiteCell> mCells;

    // grids for stats
    Grid<BACellStat> mStatsGrid;

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
