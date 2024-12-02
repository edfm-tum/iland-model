#ifndef UNDERSTOREY_H
#define UNDERSTOREY_H

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

    /// get a state by its index
    const UnderstoreyState *state(int index) const { Q_ASSERT(index>=0 && index <mStates.size()); return mStates[index]; }
    /// get a PFT by its index
    const UnderstoreyPFT *pft(int index) const { Q_ASSERT(index>=0 && index <mPFTs.size()); return mPFTs[index]; }

    /// get PFT by its name, or a nullptr if not found (slow!)
    const UnderstoreyPFT* pftByName(const QString &name) const;
    /// get a state by its id, or a nullptr if not found (slow!)
    const UnderstoreyState *stateById(UStateId id) const;

    /// get the state representing the next size class of a state (or nullptr if it is the last)
    const UnderstoreyState *nextState(const UStateId current_state) const { return state(current_state)->isFinalState() ? nullptr : state(current_state + 1); }
    /// get the state representing the previous size class of a state (or nullptr if it is already the first)
    const UnderstoreyState *previousState(UStateId current_state) const { return state(current_state)->isFirstState() ? nullptr : state(current_state - 1);}

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
    void checkStateSequence();

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
    static QStringList mVarList;
    Grid<double> mGrid;
    static UnderstoreyVisualizer *mVisualizer;


};

#endif // UNDERSTOREY_H
