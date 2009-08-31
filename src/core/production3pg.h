#ifndef PRODUCTION3PG_H
#define PRODUCTION3PG_H

class RessourceUnitSpecies;
class Production3PG
{
public:
    Production3PG();
    double calculate(RessourceUnitSpecies &rus);
};

#endif // PRODUCTION3PG_H
