#ifndef RANDOMGENERATOR_H
#define RANDOMGENERATOR_H

#include <cstdlib>
#include <cmath>
#include <vector>
#include <random> // Used for initial seeding only

// MEMORY WARNING: 2,000,000 ints * 32 threads = ~256 MB RAM.
// In this thread-local design, a size of 65536 (256KB) would actually be faster
// due to L2 cache locality, but 2M is safe if RAM is plentiful.
#define RANDOMGENERATORSIZE 2000000
#define RANDOMGENERATORROTATIONS 10

class RandomGenerator
{
public:
    enum ERandomGenerators { ergMersenneTwister, ergWellRNG512, ergXORShift96, ergFastRandom };

    // --- Constructor (Runs once per thread) ---
    RandomGenerator();

    // --- Static API (Backward Compatibility) ---
    // These functions now forward calls to the thread-local instance.

    static void setGeneratorType(const ERandomGenerators gen) {
        // Note: This only sets the type for the CURRENT thread.
        // Ideally, set this in a global config before threads start.
        instance().mGeneratorType = gen;
        instance().mRotationCount = RANDOMGENERATORROTATIONS + 1;
        instance().mIndex = 0;
        instance().mRefillCounter = 0;
    }

    static void debugState(int &rIndex, int &rGeneration, int &rRefillCount) {
        rIndex = instance().mIndex;
        rGeneration = instance().mRotationCount;
        rRefillCount = instance().mRefillCounter;
    }

    static int debugNRandomNumbers() {
        return instance().mIndex +
               RANDOMGENERATORSIZE * instance().mRotationCount +
               (RANDOMGENERATORROTATIONS + 1) * RANDOMGENERATORSIZE * instance().mRefillCounter;
    }

    static void checkGenerator() {
        if (instance().mRotationCount > RANDOMGENERATORROTATIONS) {
            instance().refill();
        }
    }

    static void setup(const ERandomGenerators gen, const unsigned oneSeed) {
        instance().mGeneratorType = gen;
        instance().seed(oneSeed); // Seeds THIS thread's generator
        instance().checkGenerator();
    }

    static void seed(const unsigned oneSeed); // Implementation in .cpp

    // --- Inlined Hot Paths (Now Lock-Free) ---

    static inline double rand() {
        return instance().next() * (1.0 / 4294967295.0);
    }

    static inline double rand(const double max_value) {
        return max_value * rand();
    }

    static inline unsigned long randInt() {
        return instance().next();
    }

    static inline unsigned long randInt(const int max_value) {
        return max_value > 0 ? randInt() % max_value : 0;
    }

    static inline double randNorm(const double mean, const double stddev);

private:
    // --- Instance Members (No longer static) ---
    // Each thread has its own buffer and state.
    unsigned int mBuffer[RANDOMGENERATORSIZE + 5];
    int mIndex = RANDOMGENERATORSIZE + 1; // Force refill on first use
    int mRotationCount = 0;
    int mRefillCounter = 0;
    ERandomGenerators mGeneratorType = ergMersenneTwister;

    // --- Instance Helper ---
    // This is the magic. It creates one RandomGenerator per thread.
    static RandomGenerator& instance() {
        static thread_local RandomGenerator rng;
        return rng;
    }

    // Internal next() function acting on the instance
    inline unsigned long next() {
        ++mIndex;
        // Branch prediction handles this well; it's false 99.999% of the time.
        if (mIndex > RANDOMGENERATORSIZE) {
            mRotationCount++;
            mIndex = 0;
            checkGenerator();
        }
        return mBuffer[mIndex];
    }

    // Must be implemented in .cpp as a member function now.
    // REMOVE LOCKS from the implementation!
    void refill();
};

// --- Global Access Functions (Unchanged) ---

inline double nrandom(const double& p1, const double& p2) {
    return p1 + RandomGenerator::rand(p2 - p1);
}

inline double drandom() {
    return RandomGenerator::rand();
}

inline int irandom(int from, int to) {
    return from + RandomGenerator::randInt(to - from);
}

inline double RandomGenerator::randNorm(const double mean, const double stddev) {
    double x, y, r;
    do {
        x = 2.0 * rand() - 1.0;
        y = 2.0 * rand() - 1.0;
        r = x * x + y * y;
    } while (r >= 1.0 || r == 0.0);
    double s = std::sqrt(-2.0 * std::log(r) / r);
    return mean + x * s * stddev;
}

#endif // RANDOMGENERATOR_H
