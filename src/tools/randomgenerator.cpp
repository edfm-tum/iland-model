/********************************************************************************************
** iLand - an individual based forest landscape and disturbance model
** https://iland-model.org
** Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#include "randomgenerator.h"
#include <QtGlobal>
#include "../3rdparty/MersenneTwister.h"
#include <chrono> // For high-res time seeding

// Internal helper class for specific RNG algorithms
class RGenerators
{
public:
    RGenerators() {}
    void seed(unsigned int oneSeed);

    inline unsigned int random_function(const int type) {
        if (type==0) return WELLRNG512();
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
inline unsigned long RGenerators::xorshf96(void) {          //period 2^96-1
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

// CRITICAL OPTIMIZATION:
// Replaced std::srand() and std::rand() with a local Linear Congruential Generator.
// std::rand() often uses a global lock in the C runtime. Calling it here would
// re-introduce the bottleneck we are trying to remove.
inline void RGenerators::seed(unsigned int oneSeed)
{
    // Local LCG to initialize the state buffer deterministically and without locks
    unsigned int local_seed = oneSeed;
    for (int i=0; i<16; i++) {
        local_seed = local_seed * 214013 + 2531011;
        state[i] = local_seed;
    }

    index = 0;
    // inits for the xorshift algorithm...
    x=123456789; y=362436069; z=521288629;
    // inits for the fast rand....
    g_seed = oneSeed;
}

// -----------------------------------------------------------------------
// RandomGenerator Implementation
// -----------------------------------------------------------------------

// This prevents the recursive call loop: instance() -> constructor -> seed() -> instance().
RandomGenerator::RandomGenerator()
{
    // Auto-seed: Needs to be unique per thread!
    // We mix high-resolution time with the memory address of this thread-local instance.
    quint64 t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    quint64 addr = reinterpret_cast<quint64>(this);

    // Direct access to members, bypassing instance()
    mBuffer[RANDOMGENERATORSIZE+4] = (unsigned int)(t ^ addr);

    // Member variables like mIndex are already initialized via in-class initializers in .h

     refill();
}

void RandomGenerator::refill() {
    // Thread-local refill - NO MUTEX required.

    mIndex = 0; // reset the index
    mRotationCount = 0;
    mRefillCounter++;

    RGenerators gen;
    // Use the last value of the buffer as the seed for the next round
    // to ensure continuity in the pseudo-random sequence.
    gen.seed(mBuffer[RANDOMGENERATORSIZE+4]);

    switch (mGeneratorType) {
    case ergMersenneTwister: {
        MTRand mersenne;
        mersenne.seed(mBuffer[RANDOMGENERATORSIZE+4]);
        for (int i=0; i<RANDOMGENERATORSIZE+5; ++i)
            mBuffer[i] = mersenne.randInt();
        break;
    }
    case ergWellRNG512: {
        for (int i=0; i<RANDOMGENERATORSIZE+5; ++i)
            mBuffer[i] = gen.random_function(0);
        break;
    }
    case ergXORShift96: {
        for (int i=0; i<RANDOMGENERATORSIZE+5; ++i)
            mBuffer[i] = gen.random_function(1);
        break;
    }
    case ergFastRandom: {
        for (int i=0; i<RANDOMGENERATORSIZE+5; ++i)
            mBuffer[i] = gen.random_function(2);
        break;
    }
    } // switch
}

void RandomGenerator::seed(const unsigned oneSeed)
{
    RandomGenerator& rng = instance();

    if (oneSeed == 0) {
        quint64 t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        quint64 addr = reinterpret_cast<quint64>(&rng);
        rng.mBuffer[RANDOMGENERATORSIZE+4] = (unsigned int)(t ^ addr);
    } else {
        rng.mBuffer[RANDOMGENERATORSIZE+4] = oneSeed;
    }
}
