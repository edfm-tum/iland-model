#ifndef SPECIESRESPONSE_H
#define SPECIESRESPONSE_H
class ResourceUnit;
class ResourceUnitSpecies;
class Species;

class SpeciesResponse
{
public:
    SpeciesResponse();
    // actions
    void setup(ResourceUnitSpecies *rus);
    /// calculate the species specific environmental response values for the resource unit.
    /// this function called before the 3pg production.
    void calculate();
    // access responses
    const double *vpdResponse() const { return mVpdResponse; }
    const double *tempResponse() const { return mTempResponse; }
    const double *soilWaterResponse() const { return mSoilWaterResponse; }
    const double *absorbedRadiation() const { return mRadiation; }
    double co2Response() const { return mCO2Response; }
    double nitrogenResponse() const { return mNitrogenResponse; }
private:
    void clear();
    const ResourceUnit *mRu;
    const Species *mSpecies;
    double mRadiation[12]; ///<  radiation sums per month (within vegetation period)
    double mVpdResponse[12];
    double mTempResponse[12];
    double mSoilWaterResponse[12];
    double mSoilWaterResponseYear;
    double mNitrogenResponse;
    double mCO2Response;
};

#endif // SPECIESRESPONSE_H
