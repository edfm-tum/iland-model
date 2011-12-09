#ifndef RANDOMWELL_H
#define RANDOMWELL_H
#include <cstdlib>
#include <math.h>

// see  http://www.lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
// for the

class MTRand
{
public:
    MTRand() {seed();}
    /// get a random value from [0., 1.]
    inline double rand() { return  double(random_function()) * (1.0/4294967295.0); }
    /// get a random value from [0., max_value]
    inline double rand(const double max_value) { return max_value * rand(); }
    /// get a random integer in [0,2^32-1]
    inline unsigned long randInt(){ return random_function(); }
    inline unsigned long randInt(const int max_value) { return randInt()%max_value; }
    /// Access to nonuniform random number distributions
    inline double randNorm( const double mean, const double stddev );
    void seed();
    void seed(unsigned int oneSeed);
private:
    inline unsigned int random_function() { return WELLRNG512();
                                            /*return xorshf96();*/ }
    /* initialize state to random bits  */
    unsigned long state[16];
    /* init should also reset this to 0 */
    unsigned int index;
    /* return 32 bit random number      */
    inline unsigned long WELLRNG512(void)
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
    unsigned long x,y,z;
    unsigned long xorshf96(void) {          //period 2^96-1
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

};


inline void MTRand::seed()
{
    srand ( time(NULL) );
    for (int i=0;i<16;i++)
        state[i] = std::rand();
    index = 0;
    x=123456789, y=362436069, z=521288629;
}

inline void MTRand::seed(unsigned int oneSeed)
{
    srand ( oneSeed );
    for (int i=0;i<16;i++)
        state[i] = std::rand();
    index = 0;
    x=123456789, y=362436069, z=521288629;
}

inline double MTRand::randNorm( const double mean, const double stddev )
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

#endif // RANDOMWELL_H
