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
    void calculate(); ///< calculate responses for current year
    void clear(); ///< set all responses to 0
    // access components
    const Species *species() const { return mSpecies; }
    const ResourceUnit *resourceUnit() const { return mRu; }
    // access responses
    const double *tempResponse() const { return mTempResponse; }
    const double *soilWaterResponse() const { return mSoilWaterResponse; }
    const double *globalRadiation() const { return mRadiation; } ///< radiation sum in MJ/m2
    const double *utilizableRadiation() const {return mUtilizableRadiation; } ///< utilizable radiation (rad*responses)
    const double *vpdResponse() const {return mVpdResponse; }
    const double *co2Response() const { return mCO2Response; }
    double nitrogenResponse() const { return mNitrogenResponse; }
    double yearlyRadiation() const { return mTotalRadiation; }
    double totalUtilizedRadiation() const { return mTotalUtilizedRadiation; }
    /// response calculation called during water cycle
    /// calculates minimum-response of vpd-response and soilwater response
    void soilAtmosphereResponses(const double psi_kPa, const double vpd, double &rMinResponse) const;

private:
    const ResourceUnit *mRu;
    const Species *mSpecies;

    double mRadiation[12]; ///<  radiation sums per month (within vegetation period, MJ/m2)
    double mUtilizableRadiation[12]; ///< sum of daily radiation*minResponse (MJ/m2)
    double mTempResponse[12]; ///< average of temperature response
    double mSoilWaterResponse[12]; ///< average of soilwater response
    double mVpdResponse[12]; ///< mean of vpd-response
    double mNitrogenResponse;
    double mCO2Response[12];
    double mTotalRadiation;  ///< total radiation of the year (MJ/m2)
    double mTotalUtilizedRadiation; ///< yearly sum of utilized radiation (MJ/m2)
};

#endif // SPECIESRESPONSE_H
