#include "random.h"
#include "global.h"
#include "expression.h"
#include "exception.h"

#include "../3rdparty/MersenneTwister.h"
Random::Random()
{
}
/** @class RandomIndex Access each index of a given size in a random order.
  Example-Usage:
  @code
  RandomIndex r(100); // create
  while (r.next())
     qDebug() << r.index(); // prints out 100 numbers (0..99) in a random order.
  @endcode
*/
RandomIndex::RandomIndex(int aCount)
{
    mField=0;
    mCount=aCount;
    if (mCount>0) {
        mField=new char[mCount];
        for (int i=0;i<mCount;i++)
            mField[i]='a';
    }
    mIndex=-1;
    mRemaining=mCount;
}

RandomIndex::~RandomIndex()
{
    if (mField)
        delete []mField;
}

bool RandomIndex::next()
{
    if (mRemaining==0) {
        mIndex=-1;
        return false;
    }
    mRemaining--;
    int random_index= irandom(0, mRemaining+1); //RandomRange(0,mRemaining+1);
    int found=0;
    for (int i=0;i<mCount;i++) {
        if (mField[i]=='a') {
            if (random_index==found) {
                mIndex=i;
                mField[i]='b';
                return true;
            }
            found++;
        }

    }
    return false;
}

/** @class RandomWeighted
  */
RandomWeighted::RandomWeighted()
{
    mSize=10;
    mMemorySize=mSize;
    mGrid=new int[mMemorySize];
}

void RandomWeighted::setup(const int gridSize)
{
    if (gridSize>mMemorySize) {
        // extend memory
        delete[] mGrid;
        mMemorySize=gridSize;
        mGrid=new int[mMemorySize];
    }

    mSize=gridSize;
    for (int i=0;i<mSize;i++)
        mGrid[i]=0;
    mMaxVal=0;
    mUpdated=false;
}

RandomWeighted::~RandomWeighted()
{
    if (mGrid)
        delete[] mGrid;
}

void RandomWeighted::setWeight(const int index, const int value)
{
    if(!mGrid || index<0 || index>=mSize)
        return;
    mGrid[index]=value;
    mUpdated=false;
}

int RandomWeighted::get()
{
    if (!mGrid)
        return -1;
    if (!mUpdated)
        updateValues();
    int rnd=irandom(0, mMaxVal);
    int index=0;
    while (rnd>=mGrid[index] && index<mSize)
        index++;
    return index;

}

double RandomWeighted::getRelWeight(const int index)
{
    // das relative gewicht der Zelle "Index".
    // das ist das Delta zu Index-1 relativ zu "MaxVal".
    if (index<0 || index>=mSize)
        return 0.;
    if (!mUpdated)
        updateValues();

    if (!mMaxVal)
        return 0;

    if (index==0)
        return mGrid[0]/double(mMaxVal);

    return (mGrid[index]-mGrid[index-1]) / double(mMaxVal);
}

double RandomWeighted::getRelWeight(const int from, const int to)
{
    // das relative gewicht der Zelle "Index".
    // das ist das Delta zu Index-1 relativ zu "MaxVal".
    if (from==to)
        return getRelWeight(from);
    if (from<0 || from>=mSize || to<0 || to>=mSize || from>to)
        return 0.;
    if (!mUpdated)
        updateValues();

    if (!mMaxVal)
        return 0.;
    return (mGrid[to]-mGrid[from]) / double(mMaxVal);
}

void RandomWeighted::updateValues()
{
    int i;
    mMaxVal=0;
    for (i=0;i<mSize;i++) {
        if (mGrid[i]!=0)
            mMaxVal+=mGrid[i];
        mGrid[i]=mMaxVal;
    }
    mUpdated=true;
}


/** @class RandomCustomPDF provide random numbers with a user defined probaility density function.
    Call setup() or use the constructor to provide a function-expression with one variable (e.g. 'x^2').
    Call get() to retrieve a random number that follows the given probabilty density function. The provided function
    is not bound to a specific value range, but should produce values below 40.
  */

RandomCustomPDF::RandomCustomPDF()
{
    mExpression=0;
}
RandomCustomPDF::~RandomCustomPDF()
{
    if (mExpression)
        delete mExpression;
}
#define BIGINTVAL 100000000
/** setup of the properites of the RandomCustomPDF.
@p funcExpr the probability density function is given as a string for an Expression. The variable (e.g. 'x')
@p lowerBound lowest possible value of the random numbers (default=0)
@p upperBound highest possible value of the random numbers (default=1)
@p isSumFunc if true, the function given in 'funcExpr' is a cumulative probabilty density function (default=false)
@p stepCount internal degree of 'slots' - the more slots, the more accurate (default=100)
  */
void RandomCustomPDF::setup(const QString &funcExpr,
                            const double lowerBound,
                            const double upperBound,
                            const bool isSumFunc,
                            const int stepCount)
{
    mSteps=stepCount;
    mSumFunction = isSumFunc;
    if (mExpression)
        delete mExpression;
    mExpression = new Expression(funcExpr);

    mRandomIndex.setup(mSteps);
    mLowerBound=lowerBound;
    mUpperBound=upperBound;
    mDeltaX = (mUpperBound-mLowerBound)/mSteps;
    double x1, x2;
    double p1, p2;
    double areaval;
    for (int i=0;i<mSteps;i++) {
        x1=mLowerBound + i*mDeltaX;
        x2=x1 + mDeltaX;
        // p1, p2: werte der pdf bei unterer und oberer grenze des aktuellen schrittes
        p1=mExpression->calculate(x1);
        p2=mExpression->calculate(x2);
        // areaval: numerische integration zwischen x1 und x2
        areaval = (p1 + p2)/2 * mDeltaX;
        if (isSumFunc)
            areaval=areaval - p1*mDeltaX; // summenwahrscheinlichkeit: nur das Delta z�hlt.
        // tsetWeightghted operiert mit integers -> umrechnung: * huge_val
        mRandomIndex.setWeight(i, int(areaval*BIGINTVAL));
    }
}

double RandomCustomPDF::get()
{
    // zufallszahl ziehen.
    if (!mExpression)
        throw IException("TRandomCustomPDF: get() without setup()!"); // not set up properly
    // (1) slot zufgetig ausw�hlen:
    int slot = mRandomIndex.get();
    // der aktuelle slot ist:
    double basevalue = mLowerBound + slot*mDeltaX;
    // (2): innerhalb des aktuellen slots gleichverteilt eine zahl ziehen
    double value = nrandom(basevalue, basevalue + mDeltaX);
    return value;
}

double RandomCustomPDF::getProbOfRange(const double lowerBound, const double upperBound)
{
    if (mSumFunction) {
        double p1,p2;
        p1 = mExpression->calculate(lowerBound);
        p2 = mExpression->calculate(upperBound);
        return p2-p1;
    }
    // Wahrscheinlichkeit, dass wert zwischen lower- und upper-bound liegt.
    if (lowerBound>upperBound)
        return 0.;
    if (lowerBound < mLowerBound || upperBound>mUpperBound)
        return 0.;
    // "steps" ist die aufl�sung innerhalb lower und upper:
    int iLow, iHigh;
    iLow = int( (mUpperBound- mLowerBound)/double(mSteps)*(lowerBound - mLowerBound) );
    iHigh = int( (mUpperBound -mLowerBound)/double(mSteps)*(upperBound - mUpperBound) );
    if (iLow<0 || iLow>=mSteps || iHigh<0 || iHigh>=mSteps)
        return -1;
    return mRandomIndex.getRelWeight(iLow, iHigh);

}
