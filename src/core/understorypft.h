#ifndef UNDERSTORYPFT_H
#define UNDERSTORYPFT_H

#include <QString>
#include "understoryplant.h"

#include "expression.h"

class CSVFile; // forward
class UnderstorySetting
{
public:
    UnderstorySetting(const CSVFile *file, int current_row): mFile(file), mRowNumber(current_row) {}
    // access
    QString error(QString msg);
    QVariant value(const QString &column_name);
    bool hasColumn(const QString &column_name);
private:
    const CSVFile *mFile;
    int mRowNumber;
};

/**
 * @brief The UnderstoryPFT class
 * stores data and logic for a single PFT
 */
class UnderstoryPFT
{
public:
    UnderstoryPFT() {};
    void setup(UnderstorySetting s, int index);
    void setFirstState(UStateId first_state) { mFirstState = first_state; }
    void setNumberOfStates(int n_states) { mNStates = n_states; }

    const QString &name() const { return mName; }
    int index() const {return mIndex; }
    UStateId firstState() const { return mFirstState; }
    /// base establishment probability should be relative small (e.g. 0.01 to 0.05).
    /// the prob scales the number resource units that are tested for establishment each year
    double baseEstablishmentProbability() const { return mBaseEstablishmentProb; }

    /// determine if the state updates
    /// based on environmental conditions
    UStateId stateTransition(const UnderstoryPlant &plant,
                             UnderstoryCellParams &ucp,
                             UnderstoryRU &us_ru) const;

    /// test for establishment for this PFT at a
    /// location with given environment factors
    bool establishment(UnderstoryCellParams &ucp,
                       double n_represented) const;

    /// return a string with useful info
    QString dump();

private:
    /// Function to translate an environmental response to pathway probabilities
    void responseToTransitionProb(const double response, double &rPrevious, double &rNext, double &rMort) const;
    QString mName;
    UStateId mFirstState { std::numeric_limits<UStateId>::max()}; ///< id of the initial state of the PFT (for establishment)
    int mNStates { 0}; ///< number of states of the pft
    int mIndex {-1}; ///< the index of the PFT in the Understory's PFT container
    double mBaseEstablishmentProb {0.}; ///< base establishment probability

    double mBaseMortalityProb {0.}; ///< base probability of a cells die-off (mortality) (prop)
    double mOptimalGrowth { 1.}; ///< time to grow under optimal env. conditions to reach the last state (years)
    double mPDecline {0. }; ///< global probability of decline (one state down) (prop.)


    // response functions for the PFT
    Expression mExprLight; ///< light response (param: corrected lif_value on the ground)
    Expression mExprNutrients; ///< nutrient response (param: available nitrogen kg/ha*yr)
    Expression mExprWater; ///< water response (param: average soil water content in veg. period)
    Expression mExprTemp; ///< temperature response (param: MAT macro or microclimate)
    Expression mExprStress; ///< function expressing "stress" as f(environment) (param: total_response)

};

/**
 * @brief The UnderstoryState class
 * represents a PFT of a single size class; this is the
 * "state" that can occupy a understory cell
 */
class UnderstoryState
{
public:
    UnderstoryState() {};
    void setup(UnderstorySetting s, int index);
    void setFirstState() { mFirstState = true; }
    void setFinalState() { mFinalState = true; }

    /// the ID of a state which is at the same time the id of the state in the Understory-container
    int id() const { return mId; }
    /// the sizeClass of the state within its PFT
    int sizeClass() const { return mSizeClass; }
    const QString &name() const { return mName; }

    bool isFirstState() const { return mFirstState; }
    bool isFinalState() const { return mFinalState; }


    int pftIndex() const { Q_ASSERT(mPFT!=nullptr); return mPFT->index(); }
    const UnderstoryPFT *pft() const { return mPFT; }
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
    const UnderstoryPFT *mPFT {nullptr};
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


#endif // UNDERSTORYPFT_H
