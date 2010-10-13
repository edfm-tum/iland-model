#ifndef FLOATINGAVERAGE_H
#define FLOATINGAVERAGE_H

/** Helper class for floating averages.
  Use add(new_value) to add a value (and get the the current average). average() returns the current average
  and sum() the total sum of stored values. Use setup() to setup place for "size" values. */
class FloatingAverage
{
public:
    FloatingAverage();
    FloatingAverage(int size) { setup(size); }
    void setup(const int size, const double InitValue = 0.);
    double add(double add_value); ///< add a value and return current average

    double average() const {return mCurrentAverage; } ///< retrieve current average
    double sum() const; ///< retrieve total sum of values.
private:
    double mCurrentAverage;
    QVector<double> mData;
    int    mSize;
    int    mPos;
    bool   mFilled;
    double mInitValue;

};
#endif // FLOATINGAVERAGE_H
