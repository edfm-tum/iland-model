#ifndef BITEDISPERSAL_H
#define BITEDISPERSAL_H

#include "biteitem.h"
#include "grid.h"
#include "scriptgrid.h"
#include "bitecellscript.h"


namespace BITE {

class BiteDispersal: public BiteItem
{
    Q_OBJECT
    Q_PROPERTY(ScriptGrid* grid READ grid)

public:
    BiteDispersal();
    Q_INVOKABLE BiteDispersal(QJSValue obj);
    void setup(BiteAgent *parent_agent);
    ScriptGrid *grid() { Q_ASSERT(mScriptGrid != nullptr); return mScriptGrid; }

    QString info();

public slots:
    // actions
    void run();
    void decide();
protected:
    QStringList allowedProperties();
private:
    /// build the dispersal kernel
    void setupKernel(QString expr, double max_dist, QString dbg_file);
    /// apply the spread kernel (probabilistically)
    void spreadKernel();
    /// prepare the grid
    void prepareGrid();
    Grid<double>  mKernel;
    int mKernelOffset;
    Grid<double> mGrid;
    ScriptGrid *mScriptGrid;
    Events mEvents;

};


} // end namespace
#endif // BITEDISPERSAL_H
