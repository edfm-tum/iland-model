#ifndef UNDERSTOREY_H
#define UNDERSTOREY_H

#include "globalsettings.h"
#include "grid.h"

#include "understoreypft.h"

/**
 * @brief The Understorey class
 * is the main object for the understorey submodule.
 *
 */
class UnderstoreyRU; // forward
class ResourceUnit; // forward
class UnderstoreyCell; // forward
class Understorey
{
public:
    Understorey();
    ~Understorey();

    /// access to the singleton
    static Understorey &instance() { Q_ASSERT(mInstance!=nullptr); return *mInstance; }

    // access
    const QVector<UnderstoreyPFT*> &PFTs() { return mPFTs; }
    const QVector<UnderstoreyState*> &states() { return mStates; }
    const UnderstoreyState *stateById(UStateId id) const;

    // access to cells
    /// return the understory cell based on the metric coordinates
    /// nullptr if not valid.
    const UnderstoreyCell *understoreyCell(QPointF metric_coord) const;

    // actions
    /// setup the understorey module
    void setup();

    /// run the understorey calculations
    void run();


private:
    static Understorey *mInstance;
    /// container of all PFTs in the system
    QVector<UnderstoreyPFT*> mPFTs;
    /// all the states (i.e., PFT in particular size class)
    QVector<UnderstoreyState*> mStates;

    /// the data for understorey; it is
    /// a vector of UnderstoreyRU, which
    /// itself is a vector of UnderstoreyCell (which contain
    /// a vector of UnderstoreyPlant)
    QVector<UnderstoreyRU> mUnderstoreyRU;
};

/**
 * @brief The UnderstoreyRU class
 * holds the actual understorey per resource unit.
 */
class UnderstoreyRU
{
public:
    // setup
    void setup();
    void setRU(ResourceUnit* ru) {mRU = ru; }

    // actions
    void calculate();

    // access
    /// get metric coordinates (landscape) of a cell with given index
    QPointF cellCoord(int index);
    /// get metric coordinates (landscape) of a cell
    QPointF cellCoord(const UnderstoreyCell& cell) { return cellCoord( &cell - mCells.begin());}
    /// get cell at given coordinates (metric)
    /// Note that selecting the right RU is
    /// done by Understorey::cell()!
    const UnderstoreyCell *cell(QPointF metric_coord) const;
private:
    ResourceUnit *mRU {0};
    std::array<UnderstoreyCell, cPxPerHectare> mCells;
};

/// Helper class to visualize microclimate data
class UnderstoreyVisualizer: public QObject {
    Q_OBJECT
public:
    UnderstoreyVisualizer(QObject *parent = nullptr);
    ~UnderstoreyVisualizer();
    static void setupVisualization();
public slots:
    //QJSValue grid(); ///< return a copy of the underlying grid
    Grid<double> *paintGrid(QString what, QStringList &names, QStringList &colors); ///< function called from iLand visualization

    static Grid<double> *grid(QString what);
private:
    Grid<double> mGrid;
    static UnderstoreyVisualizer *mVisualizer;


};


#endif // UNDERSTOREY_H
