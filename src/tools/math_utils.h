#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <cstdint>
// #include <array>


// 1. Define the available modes
enum class ExpMode {
    Std,        // std::exp (Slow, Exact)
    BitHack,     // Schraudolph (Fastest, ~3.5% error)
    Hybrid       // high-speed-math-lib approach (Fast, ~0.1% error)
};

/* test-results with i7 laptop:

BitHack:  "865.396ms" --> errors between 0 and 5%
Hybrid:   "882.159ms" --> errors much better, between 0 and -0.5% (mean -0.1%)
standard: "9774.81ms"
*/

// 2. CONFIGURATION: Change this single line to switch implementations
constexpr ExpMode CurrentExpMode = ExpMode::Hybrid;

// 3. The Implementation Details (Private/Internal)
namespace detailed {

inline double exp_bithack(double x) {
    if (x < -80.0) return 0.0;
    constexpr double a = (1LL << 52) / 0.69314718056;
    constexpr double b = (1LL << 52) * (1023.0 - 0.0);
    uint64_t val = static_cast<uint64_t>(a * x + b);
    double res;
    // C++20 std::bit_cast is preferred, but memcpy is safe in C++11/17
    __builtin_memcpy(&res, &val, sizeof(double));
    return res;
}
// Constants for ln(2)
constexpr double LN2 = 0.6931471805599453;
constexpr double INV_LN2 = 1.4426950408889634;

inline double exp_hybrid(double x) {
    // fast exp approximation using argument reduction by ln(2) and a low-order minimax polynomial
    // 1. Range Reduction
    // n = floor(x / ln(2))
    double n = std::floor(x * INV_LN2);
    // r = x - n * ln(2)
    double r = x - n * LN2;

    // 2. Calculate 2^n (using bit manipulation)
    // This is precise for integers.
    uint64_t n_bits = static_cast<uint64_t>(1023 + n);
    double pow2n;
    __builtin_memcpy(&pow2n, &(n_bits <<= 52), sizeof(double));

    // 3. 3rd-Order Minimax Polynomial for e^r
    // (r is now small, in [0, ~0.693])
    // Coefficients optimized for this range
    constexpr double c0 = 0.9999999999999999;
    constexpr double c1 = 0.999999999999917;
    constexpr double c2 = 0.5000000000021784;
    constexpr double c3 = 0.1666666666576892;

    // e^r ≈ c0 + r*(c1 + r*(c2 + r*c3))
    double r_poly = c0 + r * (c1 + r * (c2 + r * c3));

    // 4. Combine
    return pow2n * r_poly;
}
}

// 4. The Public Interface
// The compiler generates code ONLY for the selected branch.
inline double model_exp(double x) {
    if constexpr (CurrentExpMode == ExpMode::Std) {
        return std::exp(x);
    }
    else if constexpr (CurrentExpMode == ExpMode::BitHack) {
        return detailed::exp_bithack(x);
    }
    else if constexpr (CurrentExpMode == ExpMode::Hybrid) {
        return detailed::exp_hybrid(x);
    }
}

#endif // MATH_UTILS_H
