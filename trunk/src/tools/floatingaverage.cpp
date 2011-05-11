#include "floatingaverage.h"
#include <QtCore>

FloatingAverage::FloatingAverage()
{
        mCurrentAverage=0;
        mSize=0;
        mInitValue = 0.;
        mPos=-1;
}

void FloatingAverage::setup(const int size, const double InitValue)
{
       mInitValue = InitValue;
       mSize=size;
       mData.resize(mSize);
       mPos=-1;
       mCurrentAverage=0;
       mFilled=false;
       for (int i=0; i<size; i++)
           mData[i]=mInitValue;
}


double FloatingAverage::add(double add_value)
{

    mPos++;
    if (mPos>=mSize) {
        mPos=0;      // rollover again
        mFilled=true;
    }
    mData[mPos]=add_value;

    int countto=mSize;
    if (!mFilled)
        countto=mPos+1;
    double sum=0;
    for (int i=0;i<countto; i++)
        sum+=mData[i];
    if (countto)
        mCurrentAverage = sum/countto;
    else
        mCurrentAverage = mInitValue; // kann sein, wenn als erster wert 0 übergeben wird.
    return mCurrentAverage;
}

double FloatingAverage::sum() const
{
     if (mFilled)
        return mCurrentAverage * mSize;
     else
        return mCurrentAverage * (mPos+1);
}
