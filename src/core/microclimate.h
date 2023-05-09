
#ifndef MICROCLIMATE_H
#define MICROCLIMATE_H

#include "grid.h"

class ResourceUnit; // forward

// data structure for a single cell with 10m size
// the MicroclimateCell stores vegetation information
struct MicroclimateCell {
public:
    MicroclimateCell() { clear(); }
    void clear() { mConiferShare = 0; mLAI = 0; }

    /// set conifer share on the cell (0..1)
    void setConiferShare(double share) { mConiferShare = static_cast<short unsigned int>(share * 1000.); /* save as short int */ }
    /// conifer share from 0 (=0%) to 1 (=100%). Empty cells have a share of 0.
    double coniferShare() const { return static_cast<double>(mConiferShare) / 1000.; }

    /// set conifer share on the cell (0..1)
    void setLAI(double lai) { mLAI = static_cast<short unsigned int>(lai * 1000.); /* save as short int */ }
    /// conifer share from 0 (=0%) to 1 (=100%). Empty cells have a share of 0.
    double LAI() const { return static_cast<double>(mLAI) / 1000.; }

private:
    // use 16 bit per value
    short unsigned int mConiferShare;
    short unsigned int mLAI;
};

class Microclimate
{
public:
    Microclimate(const ResourceUnit *ru);
    ~Microclimate();

    /// analyze vegetation on resource unit and calculate indices
    void calculateVegetation();

    MicroclimateCell &cell(int index) { Q_ASSERT(index>=0 && index < 100); return mCells[index]; }
    const MicroclimateCell &constCell(int index) const { Q_ASSERT(index>=0 && index < 100); return mCells[index]; }
    int cellIndex(const QPointF &coord);
private:
    const ResourceUnit *mRU;
    MicroclimateCell *mCells;
};

class MicroclimateVisualizer: public QObject {
    Q_OBJECT
public:
    MicroclimateVisualizer(QObject *parent = nullptr);
    ~MicroclimateVisualizer();
    static void setupVisualization();
public slots:
    //QJSValue grid(); ///< return a copy of the underlying grid
    Grid<double> *paintGrid(QString what, QStringList &names, QStringList &colors); ///< function called from iLand visualization
private:
    Grid<double> mGrid;
    static MicroclimateVisualizer *mVisualizer;

};

#endif // MICROCLIMATE_H
