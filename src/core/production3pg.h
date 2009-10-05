#ifndef PRODUCTION3PG_H
#define PRODUCTION3PG_H

class SpeciesResponse;
class Production3PG
{
public:
    Production3PG();
    void setResponse(const SpeciesResponse *reponse) { mResponse=response;}
    double calculate(); ///< return  year GPP/rad: gC / (yearly MJ/m2)
    double harshness() const { return mHarshness; } /// 3PG-harshness factor used to determine how much biomass should be distributed to roots
    double GPPperRad() const { return mGPPperRad; }
private:
    inline double calculateUtilizablePAR(const int month);
    inline double calculateEpsilon(const int month);
    const SpeciesResponse *mResponse;
    double mAlphaC[12];
    double mGPP[12];
    double mNPP[12];
    double mHarshness;
    double mGPPperRad;
};

#endif // PRODUCTION3PG_H
