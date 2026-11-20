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
#include "randomgenerator.h"
#include <QtGlobal>
#include "../3rdparty/MersenneTwister.h"

#include <QMutex>

class RGenerators
{
public:
    RGenerators() {}
    void seed();
    void seed(unsigned int oneSeed);
    inline unsigned int random_function(const int type) {   if (type==0) return WELLRNG512();
                                                            if (type==1) return xorshf96();
                                                            if (type==2) return fastrand();
                                                            return 0;
                                                        }
private:
    // see  http://www.lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
    // for details on the WellRNG512 algorithm
    inline unsigned long WELLRNG512(void);
    /* initialize state to random bits  */
    unsigned long state[16];
    /* init should also reset this to 0 */
    unsigned int index;
    /* return 32 bit random number      */

    int g_seed;
    inline unsigned int fastrand()
    {
        g_seed = (214013*g_seed+2531011);
        return g_seed;
    }
    unsigned long x,y,z;
    unsigned long xorshf96(void);

};

inline unsigned long RGenerators::WELLRNG512(void)
{
unsigned long a, b, c, d;
a  = state[index];
c  = state[(index+13)&15];
b  = a^c^(a<<16)^(c<<15);
c  = state[(index+9)&15];
c ^= (c>>11);
a  = state[index] = b^c;
d  = a^((a<<5)&0xDA442D24UL);
index = (index + 15)&15;
a  = state[index];
state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
return state[index];
}
// The Marsaglia's xorshf generator:
// see: http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c and
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
//unsigned long x=123456789, y=362436069, z=521288629;
unsigned long RGenerators::xorshf96(void) {          //period 2^96-1
unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}


inline void RGenerators::seed()
{
    srand ( time(NULL) );
    for (int i=0;i<16;i++)
        state[i] = std::rand();
    index = 0;
    // inits for the xorshift algorithm...
    x=123456789, y=362436069, z=521288629;
    // inits for the fast rand....
    g_seed = std::rand();
}

inline void RGenerators::seed(unsigned int oneSeed)
{
    srand ( oneSeed );
    for (int i=0;i<16;i++)
        state[i] = std::rand();
    index = 0;
    x=123456789, y=362436069, z=521288629;
    g_seed = oneSeed;
}

// static variables
unsigned int RandomGenerator::mBuffer[RANDOMGENERATORSIZE+5];
int RandomGenerator::mIndex = 0;
thread_local int RandomGenerator::tIndex = 0;
thread_local int RandomGenerator::tEpoch = -1;
thread_local int RandomGenerator::tSlot = -1;
thread_local int RandomGenerator::tLocalPos = RANDOMGENERATORLOCALSIZE;
thread_local unsigned int RandomGenerator::tLocal[RANDOMGENERATORLOCALSIZE];
alignas(64) std::atomic<int> RandomGenerator::mEpoch{0};
std::atomic<int> RandomGenerator::mThreadSlots{0};
static thread_local MTRand *tLocalGenerator = nullptr; // generator of a non-main thread (one per thread, kept for the thread's life)

static inline unsigned long long splitmix64(unsigned long long x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

void RandomGenerator::startEpoch(int epoch)
{
    tEpoch = epoch;
    if (tSlot < 0)
        tSlot = mThreadSlots.fetch_add(1, std::memory_order_relaxed); // first thread: slot 0
    if (tSlot == 0) {
        tIndex = 0; // the main thread reads the shared buffer from the start, as upstream
        return;
    }
    // a non-main thread: own Mersenne Twister, seeded from the run's seed (the seed of the
    // next shared refill), the thread's slot and the generation
    unsigned long long key = (static_cast<unsigned long long>(mBuffer[RANDOMGENERATORSIZE+4]) << 32)
                             ^ (static_cast<unsigned long long>(tSlot) << 16) ^ static_cast<unsigned long long>(epoch);
    if (!tLocalGenerator)
        tLocalGenerator = new MTRand(static_cast<MTRand::uint32>(splitmix64(key)));
    else
        tLocalGenerator->seed(static_cast<MTRand::uint32>(splitmix64(key)));
    tLocalPos = RANDOMGENERATORLOCALSIZE; // refill on the next draw
}

void RandomGenerator::refillLocal()
{
    for (int i = 0; i < RANDOMGENERATORLOCALSIZE; ++i)
        tLocal[i] = tLocalGenerator->randInt();
    tLocalPos = 0;
}
int RandomGenerator::mRotationCount = RANDOMGENERATORROTATIONS + 1;
int RandomGenerator::mRefillCounter = 0;
RandomGenerator::ERandomGenerators RandomGenerator::mGeneratorType = RandomGenerator::ergFastRandom;



/// fill the internal buffer with random numbers choosen from the defined random generator
QMutex random_generator_refill_mutex;

void RandomGenerator::refill() {

    {
    // only one thread should refill the random number buffer, but we do
    // allow the other threads to continue taking random numbers while refilling
    QMutexLocker lock(&random_generator_refill_mutex); // serialize access
    if (mRotationCount<RANDOMGENERATORROTATIONS) // another thread might already succeeded in refilling....
        return;
    mIndex = 0; // reset the index
    mRotationCount=0;
    mRefillCounter++;
    mEpoch.fetch_add(1, std::memory_order_relaxed); // every thread restarts its stream
    }

    RGenerators gen;
    gen.seed(mBuffer[RANDOMGENERATORSIZE+4]); // use the last value as seed for the next round....
    switch (mGeneratorType) {
    case ergMersenneTwister: {
        MTRand mersenne;
        // qDebug() << "refill random numbers. seed" <<mBuffer[RANDOMGENERATORSIZE+4];
        mersenne.seed(mBuffer[RANDOMGENERATORSIZE+4]);
        //mersenne.seed(); // use random number seed from mersenne twister (hash of time and clock)
        for (int i=0;i<RANDOMGENERATORSIZE+5;++i)
            mBuffer[i] = mersenne.randInt();
        break;
    }
    case ergWellRNG512: {

        for (int i=0;i<RANDOMGENERATORSIZE+5;++i)
            mBuffer[i] = gen.random_function(0);
        break;
    }
    case ergXORShift96: {
        for (int i=0;i<RANDOMGENERATORSIZE+5;++i)
            mBuffer[i] = gen.random_function(1);
        break;
    }
    case ergFastRandom: {
        for (int i=0;i<RANDOMGENERATORSIZE+5;++i)
            mBuffer[i] = gen.random_function(2);
        break;
    }
    } // switch


}

void RandomGenerator::seed(const unsigned oneSeed)
{
    if (oneSeed==0) {
        srand ( time(NULL) );
        mBuffer[RANDOMGENERATORSIZE+4] = std::rand();
    } else {
        mBuffer[RANDOMGENERATORSIZE+4] = oneSeed; // set a specific seed as seed for the next round
    }
}
