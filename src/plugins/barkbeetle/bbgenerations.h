#ifndef BBGENERATIONS_H
#define BBGENERATIONS_H

#include <QVector>

class ResourceUnit; // forward

class BBGenerations
{
public:
    BBGenerations();
    /// calculate the number of barbeetle generations for the given resource unit.
    double calculateGenerations(const ResourceUnit *ru);

    /// number of sister broods (reaching at least 60% of thermal development)
    int sisterBroods() const { return mNSisterBroods; }
    /// number consecutive broods (reaching at least 60% of thermal development)
    int filialBroods() const { return mNFilialBroods; }

    /// returns true, if the sister broods of the same generation were also developed
    /// (e.g. 2 gen + 2 sister -> true, 2 gen + 1 sister -> false)
    bool hasSisterBrood() const { return mNSisterBroods == mNFilialBroods && mNSisterBroods>0; }

    /// number of cold days (tmin < -15 degrees) in the first half of the year
    int frostDaysEarly() const { return mFrostDaysEarly; }
    /// number of cold days (tmin < -15 degrees) in the second half of the year
    int frostDaysLate() const { return mFrostDaysLate; }

private:
    void calculateBarkTemperature(const ResourceUnit *ru);
    struct BBGeneration {
        BBGeneration(): start_day(-1), gen(0), is_sister_brood(false), value(0.) {}
        BBGeneration(int start, bool is_filial, int generation) { start_day=start; is_sister_brood=is_filial; value=0.; gen=generation; }
        int start_day;
        int gen;
        bool is_sister_brood;
        double value;
    };
    QVector<BBGeneration> mGenerations;
    int mNSisterBroods; ///< number of sister broods (reaching at least 60% of thermal development)
    int mNFilialBroods; ///< number consecutive broods (reaching at least 60% of thermal development)
    int mFrostDaysEarly; ///< frost days (tmin<-15) from Jan 1 to summer
    int mFrostDaysLate; ///< frost days from summer to Dec 31

    double mEffectiveBarkTemp[366];
};

#endif // BBGENERATIONS_H
