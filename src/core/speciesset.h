#ifndef SPECIESSET_H
#define SPECIESSET_H
#include <QtSql>

#include "stampcontainer.h"
#include "expression.h"
class Species;
class SeedDispersal;

class SpeciesSet
{
public:
    SpeciesSet();
    ~SpeciesSet();
    const QString &name() const { return mName; } ///< table name of the species set
    // access
    QList<Species*> activeSpecies() { return mActiveSpecies; }
    Species *species(const QString &speciesId) { return mSpecies.value(speciesId); }
    const Species *species(const int &index); ///< get by arbirtray index (slower than using string-id!)
    const StampContainer &readerStamps() { return mReaderStamp; }
    QVariant var(const QString& varName);
    int count() const { return mSpecies.count(); }
    // calculations
    double nitrogenResponse(const double availableNitrogen, const double &responseClass) const;
    double co2Response(const double ambientCO2, const double nitrogenResponse, const double soilWaterResponse) const;
    double lightResponse(const double lightResourceIndex, const double lightResponseClass) const;
    double LRIcorrection(const double lightResourceIndex, const double relativeHeight) const  { return mLRICorrection.calculate(lightResourceIndex, relativeHeight);}
    // maintenance
    void clear();
    int setup();
    void setupRegeneration(); ///< setup of regenartion related data
    // running
    void newYear(); ///< is called at the beginning of a year
    void regeneration(); ///< run regeneration (after growth)
private:
    QString mName;
    double nitrogenResponse(const double &availableNitrogen, const double &NA, const double &NB) const;
    QList<Species*> mActiveSpecies; ///< list of species that are "active" (flag active in database)
    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
    StampContainer mReaderStamp;
    // nitrogen response classes
    double mNitrogen_1a, mNitrogen_1b; ///< parameters of nitrogen response class 1
    double mNitrogen_2a, mNitrogen_2b; ///< parameters of nitrogen response class 2
    double mNitrogen_3a, mNitrogen_3b; ///< parameters of nitrogen response class 3
    // CO2 response
    double mCO2base, mCO2comp; ///< CO2 concentration of measurements (base) and CO2 compensation point (comp)
    double mCO2p0, mCO2beta0; ///< p0: production multiplier, beta0: relative productivity increase
    // Light Response classes
    Expression mLightResponseIntolerant; ///< light response function for the the most shade tolerant species
    Expression mLightResponseTolerant; ///< light response function for the most shade intolerant species
    Expression mLRICorrection; ///< function to modfiy LRI during read
    /// container holding the seed maps
    QList<SeedDispersal*> mSeedDispersal;

};

#endif // SPECIESSET_H
