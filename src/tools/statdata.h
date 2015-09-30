#ifndef STATDATA_H
#define STATDATA_H

/** StatData.
* Helper class for statistics. This class calculates
* from a double-vector relevant information used
* for BoxPlots. */
class StatData
{
public:
    StatData() { calculate(); }
    StatData(QVector<double> &data);
    void setData(QVector<double> &data) { mData=data; calculate(); }
    void calculate();
    // getters
    double sum() const { return mSum; } ///< sum of values
    double mean() const { return mMean; } ///< arithmetic mean
    double min() const { return mMin; } ///< minimum value
    double max() const { return mMax; } ///< maximum value
    double median() const { if (mP25==std::numeric_limits<double>::max()) calculatePercentiles(); return mMedian; } ///< 2nd quartil = median
    double percentile25() const { if (mP25==std::numeric_limits<double>::max()) calculatePercentiles(); return mP25; } ///< 1st quartil
    double percentile75() const { if (mP25==std::numeric_limits<double>::max()) calculatePercentiles(); return mP75; } ///< 3rd quartil
    double percentile(const int percent) const; ///< get value of a given percentile (0..100)
    double standardDev() const { if (mSD==std::numeric_limits<double>::max()) calculateSD(); return mSD; } ///< get the standard deviation (of the population)
    // additional functions
    static QVector<int> calculateRanks(const QVector<double> &data, bool descending=false); ///< rank data.
    static void normalize(QVector<double> &data, double targetSum); ///< normalize, i.e. the sum of all items after processing is targetSum
private:
    double calculateSD() const;
    mutable QVector<double> mData; // mutable to allow late calculation of percentiles (e.g. a call to "median()".)
    double mSum;
    double mMean;
    double mMin;
    double mMax;
    mutable double mP25;
    mutable double mP75;
    mutable double mMedian;
    mutable double mSD; // standard deviation
    void calculatePercentiles() const;
};


#endif // STATDATA_H
