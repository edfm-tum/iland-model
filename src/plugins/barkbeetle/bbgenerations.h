#ifndef BBGENERATIONS_H
#define BBGENERATIONS_H

class BBGenerations
{
public:
    BBGenerations();
private:
    void calculateBarkTemperature(const ResourceUnit *ru);
    double mBarkTemp[366];
};

#endif // BBGENERATIONS_H
