#ifndef SPECIESSET_H
#define SPECIESSET_H
#include <QtSql>

#include "stampcontainer.h"
#include "phenology.h"
class Species;

class SpeciesSet
{
public:
    SpeciesSet();
    ~SpeciesSet();
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
    const Phenology &phenology(const int phenologyGroup);
    // maintenance
    void clear();
    int setup();
private:
    double nitrogenResponse(const double &availableNitrogen, const double &NA, const double &NB) const;
    void setupPhenology();
    QList<Species*> mActiveSpecies;
    QMap<QString, Species*> mSpecies;
    QSqlQuery *mSetupQuery;
    QList<Phenology> mPhenology;
    StampContainer mReaderStamp;
    // nitrogen response classes
    double mNitrogen_1a, mNitrogen_1b; ///< parameters of nitrogen response class 1
    double mNitrogen_2a, mNitrogen_2b; ///< parameters of nitrogen response class 2
    double mNitrogen_3a, mNitrogen_3b; ///< parameters of nitrogen response class 3
    // CO2 response
    double mCO2base, mCO2comp; ///< CO2 concentration of measurements (base) and CO2 compensation point (comp)
    double mCO2p0, mCO2beta0; ///< p0: production multiplier, beta0: relative productivity increase
};

#endif // SPECIESSET_H
