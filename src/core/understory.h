#ifndef UNDERSTORY_H
#define UNDERSTORY_H

#include "grid.h"

#include "understorypft.h"

/**
 * @brief The Understory class
 * is the main object for the understory submodule.
 *
 */
class UnderstoryRU; // forward
class ResourceUnit; // forward
class UnderstoryCell; // forward
class Understory
{
public:
    Understory();
    ~Understory();

    /// access to the singleton
    static Understory &instance() { Q_ASSERT(mInstance!=nullptr); return *mInstance; }

    // access
    const QVector<UnderstoryPFT*> &PFTs() { return mPFTs; }
    const QVector<UnderstoryState*> &states() { return mStates; }

    /// get a state by its index/Id. plant->StateId works (0-based)
    const UnderstoryState *state(int state_id) const { Q_ASSERT(state_id>=0 && state_id <mStates.size()); return mStates[state_id]; }
    /// get a PFT by its index
    const UnderstoryPFT *pft(int index) const { Q_ASSERT(index>=0 && index <mPFTs.size()); return mPFTs[index]; }

    /// get PFT by its name, or a nullptr if not found (slow!)
    const UnderstoryPFT* pftByName(const QString &name) const;

    // get a state by its id, or a nullptr if not found (slow!)
    //const UnderstoryState *stateById(UStateId id) const;

    /// get the state representing the next size class of a state (or nullptr if it is the last)
    const UnderstoryState *nextState(const UStateId current_state) const { return state(current_state)->isFinalState() ? nullptr : state(current_state + 1); }
    /// get the state representing the previous size class of a state (or nullptr if it is already the first)
    const UnderstoryState *previousState(UStateId current_state) const { return state(current_state)->isFirstState() ? nullptr : state(current_state - 1);}

    // access to cells
    /// return the understory cell based on the metric coordinates
    /// nullptr if not valid.
    const UnderstoryCell *understoryCell(QPointF metric_coord) const;

    /// get the RU container for given coordinates (or nullptr if not valid)
    const UnderstoryRU *understoryRU(QPointF metric_coord) const;

    /// get the RU container for given index
    UnderstoryRU *understoryRU(int index)  { return &mUnderstoryRU[index]; }

    // actions
    /// setup the understory module
    void setup();

    bool isValid() const { return !mUnderstoryRU.isEmpty();}



private:
    void checkStateSequence();

    static Understory *mInstance;
    /// container of all PFTs in the system
    QVector<UnderstoryPFT*> mPFTs;
    /// all the states (i.e., PFT in particular size class)
    QVector<UnderstoryState*> mStates;

    /// the data for understory; it is
    /// a vector of UnderstoryRU, which
    /// itself is a vector of UnderstoryCell (which contain
    /// a vector of UnderstoryPlant)
    QVector<UnderstoryRU> mUnderstoryRU;

    friend class UnderstoryOut;

};


/// Helper class to visualize microclimate data
class UnderstoryVisualizer: public QObject {
    Q_OBJECT
public:
    UnderstoryVisualizer(QObject *parent = nullptr);
    ~UnderstoryVisualizer();
    static void setupVisualization();
public slots:
    //QJSValue grid(); ///< return a copy of the underlying grid
    Grid<double> *paintGrid(QString what, QStringList &names, QStringList &colors); ///< function called from iLand visualization

    void filterChanged(int filter_index);

    static Grid<double> *grid(QString what);

private:
    static QStringList mVarList;
    Grid<double> mGrid; // detailed 2m grid
    Grid<double> mRUGrid; // RU level grid (100m)
    UnderstoryPFT *mPFTFilter { nullptr }; // PFT ptr for filtered display
    static UnderstoryVisualizer *mVisualizer;


};

#endif // UNDERSTORY_H
