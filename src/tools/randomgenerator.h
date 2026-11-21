/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef RANDOMGENERATOR_H
#define RANDOMGENERATOR_H
#include <cstdlib>
#include <math.h>
#include <time.h>
#include <atomic>

#define RANDOMGENERATORSIZE 2000000
#define RANDOMGENERATORROTATIONS 10
#define RANDOMGENERATORLOCALSIZE 1024 // per-thread buffer of the non-main threads
// a new set of numbers is generated for every 5*500000 = 2.500.000 numbers
class RandomGenerator
{
public:
    enum ERandomGenerators { ergMersenneTwister, ergWellRNG512, ergXORShift96, ergFastRandom };
    RandomGenerator() { seed(0); setGeneratorType(ergMersenneTwister); }
    /// set the type of the random generator that should be used.
    static void setGeneratorType(const ERandomGenerators gen) { mGeneratorType = gen; mRotationCount=RANDOMGENERATORROTATIONS+1;mIndex=0;mRefillCounter=0; mEpoch.fetch_add(1, std::memory_order_relaxed); }
    static void debugState(int &rIndex, int &rGeneration, int &rRefillCount) { rIndex = tIndex; rGeneration = mRotationCount; rRefillCount = mRefillCounter; }
    static int debugNRandomNumbers() { return tIndex + RANDOMGENERATORSIZE*mRotationCount + (RANDOMGENERATORROTATIONS+1)*RANDOMGENERATORSIZE*mRefillCounter; }
    /// call this function to check if we need to create new random numbers.
    /// this function is not reentrant! (e.g. call every year in the model)
    static void checkGenerator() { if (mRotationCount>RANDOMGENERATORROTATIONS) { refill();  } }
    static void setup(const ERandomGenerators gen, const unsigned oneSeed) { setGeneratorType(gen); seed(oneSeed); checkGenerator(); }
    /// set a random generator seed. If oneSeed is 0, then a random number (provided by system time) is used as initial seed.
    static void seed(const unsigned oneSeed);
    /// get a random value from [0., 1.]
    static inline double rand() { return next() * (1.0/4294967295.0); }
    static inline double rand(const double max_value) { return max_value * rand(); }
    /// get a random integer in [0,2^32-1]
    static inline unsigned long randInt(){ return next(); }
    static inline unsigned long randInt(const int max_value) { return max_value>0?randInt()%max_value:0; }
    /// Access to nonuniform random number distributions
    static inline double randNorm( const double mean, const double stddev );

private:
    // Threads do not share an index any more. Upstream, every call from every thread wrote the
    // same global index, and with many threads (e.g. in Saplings::establishment) the threads spent
    // most of their time waiting for that one cache line.
    // - The first thread that draws numbers (the main thread, during setup) reads the shared
    //   buffer exactly as before, so single-threaded runs draw the same sequence as upstream.
    // - Every other thread has its own Mersenne Twister, seeded from the run's seed, the
    //   thread's slot and the buffer generation (mEpoch), and keeps a small local buffer.
    //   No numbers are shared between threads, and no shared state is written.
    // mEpoch changes whenever the shared buffer is refilled or reset (new seed).
    static inline unsigned long next() {
        const int epoch = mEpoch.load(std::memory_order_relaxed);
        if (tEpoch != epoch) startEpoch(epoch);
        if (tSlot != 0) {
            if (tLocalPos >= RANDOMGENERATORLOCALSIZE) refillLocal();
            return tLocal[tLocalPos++];
        }
        ++tIndex;
        if (tIndex>RANDOMGENERATORSIZE) { mRotationCount++; tIndex=0; checkGenerator(); }
        return mBuffer[tIndex];
    }
    static void startEpoch(int epoch); ///< (re)start the calling thread's stream for buffer generation 'epoch'
    static void refillLocal(); ///< fill the calling thread's local buffer from its own generator
    static unsigned int mBuffer[RANDOMGENERATORSIZE+5];
    static int mIndex; ///< kept for the refill bookkeeping; the main thread reads with tIndex
    static thread_local int tIndex;
    static thread_local int tEpoch;
    static thread_local int tSlot;
    static thread_local int tLocalPos;
    static thread_local unsigned int tLocal[RANDOMGENERATORLOCALSIZE];
    alignas(64) static std::atomic<int> mEpoch;
    static std::atomic<int> mThreadSlots;
    static int mRotationCount;
    static int mRefillCounter;
    static ERandomGenerators mGeneratorType;
    static void refill();
};

/// ******************************************
/// Global access functions for random numbers
/// ******************************************


/// nrandom returns a random number from [p1, p2] -> p2 is a possible result!
inline double nrandom(const double& p1, const double& p2)
{
    return p1 + RandomGenerator::rand(p2-p1);
    //return p1 + (p2-p1)*(rand()/double(RAND_MAX));
}
/// returns a random number in [0,1] (i.e.="1" is a possible result!)
inline double drandom()
{
    return RandomGenerator::rand();
    //return rand()/double(RAND_MAX);
}
/// return a random number from "from" to "to" (excluding 'to'.), i.e. irandom(3,6) results in 3, 4 or 5.
inline int irandom(int from, int to)
{
    return from + RandomGenerator::randInt(to-from);
    //return from +  rand()%(to-from);
}




inline double RandomGenerator::randNorm( const double mean, const double stddev )
{
        // Return a real number from a normal (Gaussian) distribution with given
        // mean and standard deviation by polar form of Box-Muller transformation
        double x, y, r;
        do
        {
                x = 2.0 * rand() - 1.0;
                y = 2.0 * rand() - 1.0;
                r = x * x + y * y;
        }
        while ( r >= 1.0 || r == 0.0 );
        double s = sqrt( -2.0 * log(r) / r );
        return mean + x * s * stddev;
}








#endif // RANDOMGENERATOR_H
