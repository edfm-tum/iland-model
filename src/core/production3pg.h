#ifndef PRODUCTION3PG_H
#define PRODUCTION3PG_H

class SpeciesResponse;
class ProductionOut;
class Production3PG
{
public:
    Production3PG();
    void setResponse(const SpeciesResponse *response) { mResponse=response;}
    double calculate(); ///< return  year GPP/rad: kg Biomass/MJ PAR/m2
    double rootFraction() const { return mRootFraction; } /// fraction of biomass that should be distributed to roots
    double GPPperRad() const { return mGPPperRad; } ///< get GPP production per MJ radiaton (kg Biomass)
private:
    inline double calculateUtilizablePAR(const int month) const;
    inline double calculateEpsilon(const int month) const;
    inline double abovegroundFraction() const; ///< calculate fraction of biomass
    const SpeciesResponse *mResponse; ///< species specific responses
    double mUPAR[12]; ///< utilizable radiation MJ
    double mGPP[12]; ///< monthly Gross Primary Production gC/MJ radiation
    double mRootFraction; ///< fraction of production that flows into roots
    double mGPPperRad; ///< kg GPP Biomass / MJ PAR

    friend class ProductionOut;
};

#endif // PRODUCTION3PG_H
