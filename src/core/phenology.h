#ifndef PHENOLOGY_H
#define PHENOLOGY_H

class Phenology
{
public:
    Phenology() { mId=0; mMinVpd=mMaxVpd=mMinDayLength=mMaxDayLength=mMinTemp=mMaxTemp=0.; }
    Phenology(const int id, const double minVpd, const double maxVpd,
              const double minDayLength, const double maxDayLength,
              const double minTemp, const double maxTemp): mId(id), mMinVpd(minVpd), mMaxVpd(maxVpd),
                                mMinDayLength(minDayLength), mMaxDayLength(maxDayLength), mMinTemp(minTemp), mMaxTemp(maxTemp) {}
    // access
    const int id() const { return mId; }
    /// calculate the phenology for the current year
    void calculate();
    /// get result of phenology calcualtion for this year (a pointer to a array of 12 values)
    const double *month() const { return mPhenoFraction; }


private:
    int mId; ///< identifier of this Phenology group
    double mMinVpd; ///< minimum vpd [kPa]
    double mMaxVpd; ///< maximum vpd [kPa]
    double mMinDayLength; ///< minimum daylength [hours]
    double mMaxDayLength; ///< maximum daylength [hours]
    double mMinTemp; ///< minimum temperature [°]
    double mMaxTemp; ///< maximum temperature [°]
    double mPhenoFraction[12]; ///< fraction [0..1] of month i [0..11] to are inside the vegetation period, i.e. have leafs
};

#endif // PHENOLOGY_H
