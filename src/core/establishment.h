#ifndef ESTABLISHMENT_H
#define ESTABLISHMENT_H
class Climate;
class ResourceUnitSpecies;
class Establishment
{
public:
    Establishment(const Climate *climate, const ResourceUnitSpecies *rus);
    void calculate();
private:
    double mPAbiotic; ///< abiotic probability for establishment (climate)
    void calculateAbioticEnvironment(); ///< calculate the abiotic environment (TACA model)
    const Climate *mClimate; ///< link to the current climate
    const ResourceUnitSpecies *mRUS; ///< link to the resource unit species (links to production data and species respones)
    double mRegenerationProbability; ///< prob. of regeneration in the current year
};

#endif // ESTABLISHMENT_H
