#ifndef UNDERSTOREYPFT_H
#define UNDERSTOREYPFT_H

#include <QString>
#include "understoreyplant.h"

#include "expression.h"

class CSVFile; // forward
class UnderstoreySetting
{
public:
    UnderstoreySetting(const CSVFile *file, int current_row): mFile(file), mRowNumber(current_row) {}
    // access
    QString error(QString msg);
    QVariant value(const QString &column_name);
    bool hasColumn(const QString &column_name);
private:
    const CSVFile *mFile;
    int mRowNumber;
};

/**
 * @brief The UnderstoreyPFT class
 * stores data and logic for a single PFT
 */
class UnderstoreyPFT
{
public:
    UnderstoreyPFT() {};
    void setup(UnderstoreySetting s, int index);
    void setFirstState(UStateId first_state) { mFirstState = first_state; }

    const QString &name() const { return mName; }
    int index() const {return mIndex; }
    UStateId firstState() const { return mFirstState; }

    /// determine if the state updates
    /// based on environmental conditions
    UStateId stateTransition(const UnderstoreyPlant &plant,
                             UnderstoreyCellParams &ucp,
                             UnderstoreyRUStats &rustats) const;

    bool establishment(UnderstoreyCellParams &ucp,
                       UnderstoreyRUStats &rustats) const;

private:
    QString mName;
    UStateId mFirstState { std::numeric_limits<UStateId>::max()}; ///< id of the initial state of the PFT (for establishment)
    int mIndex {-1}; ///< the index of the PFT in the Understorey's PFT container

    // response functions for the PFT
    Expression mExprLight; ///< light response (param: corrected lif_value on the ground)
    Expression mExprNutrients; ///< nutrient response (param: available nitrogen kg/ha*yr)
    Expression mExprWater; ///< water response (param: average soil water content in veg. period)

};

/**
 * @brief The UnderstoreyState class
 * represents a PFT of a single size class; this is the
 * "state" that can occupy a understorey cell
 */
class UnderstoreyState
{
public:
    UnderstoreyState() {};
    void setup(UnderstoreySetting s, int index);
    void setFirstState() { mFirstState = true; }
    void setFinalState() { mFinalState = true; }

    /// the ID of a state which is at the same time the id of the state in the Understorey-container
    int id() const { return mId; }
    /// the sizeClass of the state within its PFT
    int sizeClass() const { return mSizeClass; }
    const QString &name() const { return mName; }

    bool isFirstState() const { return mFirstState; }
    bool isFinalState() const { return mFinalState; }


    int pftIndex() const { Q_ASSERT(mPFT!=nullptr); return mPFT->index(); }
    const UnderstoreyPFT *pft() const { return mPFT; }
    /// number of slots that are occupied by
    /// this state
    short int NSlots() const { return mNSlots; }
    /// leaf area index (m2/m2)
    double LAI() const { return mLAI; }
    /// plant biomass of all plants of the state on the cell (g/m2)
    double biomass() const { return mBiomass; }
    /// mean height (m)
    double height() const { return mHeight; }
    /// proportion of cover on the cell (0..1)
    double cover() const { return mCover; }


private:
    const UnderstoreyPFT *mPFT {nullptr};
    int mId {-1};
    int mSizeClass;
    QString mName;
    bool mFirstState {false};
    bool mFinalState {false};

    short int mNSlots {1};
    double mLAI {0.};
    double mBiomass {0.};
    double mHeight {0.};
    double mCover {0.};


};


#endif // UNDERSTOREYPFT_H
