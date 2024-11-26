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
    UnderstoreyPFT();
    const QString &name() const { return mName; }
private:
    QString mName;
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
    void setup(UnderstoreySetting s);
    UStateId id() const { return mId; }
    const QString &name() const { return mName; }
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
    UnderstoreyPFT *mPFT {nullptr};
    UStateId mId {0};
    QString mName;
    short int mNSlots {1};
    double mLAI {0.};
    double mBiomass {0.};
    double mHeight {0.};
    double mCover {0.};

    Expression mExprLight;
    Expression mExprNutrients;
    Expression mExprWater;

};


#endif // UNDERSTOREYPFT_H
