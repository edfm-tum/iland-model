#ifndef PRODUCTION3PG_H
#define PRODUCTION3PG_H

class ResourceUnitSpecies;
class Production3PG
{
public:
    Production3PG();
    double calculate(); ///< return  year GPP/rad: gC / (yearly MJ/m2)
    double harshness() const { return mHarshness; } /// 3PG-harshness factor used to determine how much biomass should be distributed to roots
    double GPPperRad() const { return mGPPperRad; }
private:
    double mHarshness;
    double mGPPperRad;
};

#endif // PRODUCTION3PG_H
