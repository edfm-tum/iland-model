#ifndef BBGENERATIONS_H
#define BBGENERATIONS_H

#include <QVector>

class ResourceUnit; // forward

class BBGenerations
{
public:
    BBGenerations();
    double calculateGenerations(const ResourceUnit *ru);
private:
    void calculateBarkTemperature(const ResourceUnit *ru);
    struct BBGeneration {
        BBGeneration(): start_day(-1), gen(0), is_filial_generation(false), value(0.) {}
        BBGeneration(int start, bool is_filial, int generation) { start_day=start; is_filial_generation=is_filial; value=0.; gen=generation; }
        int start_day;
        int gen;
        bool is_filial_generation;
        double value;
    };
    QVector<BBGeneration> mGenerations;

    double mEffectiveBarkTemp[366];
};

#endif // BBGENERATIONS_H
