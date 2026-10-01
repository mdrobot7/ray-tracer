#pragma once

#include <cmath>
#include <cstdlib>
#include <random>

#define EPSILON (1e-8)
#define CLOSE_TO(a, b) (std::abs(a - b) <= EPSILON)

// In range, inclusive
#define IN_RANGE(x, min, max) ((x) >= (min) && (x) <= (max))

#define MAX(a, b)                                                                                  \
    ({                                                                                             \
        __typeof__(a) _a = (a);                                                                    \
        __typeof__(b) _b = (b);                                                                    \
        _a > _b ? _a : _b;                                                                         \
    })

#define MIN(a, b)                                                                                  \
    ({                                                                                             \
        __typeof__(a) _a = (a);                                                                    \
        __typeof__(b) _b = (b);                                                                    \
        _a < _b ? _a : _b;                                                                         \
    })

// Clamp value to within the range [lower, upper], saturating at
// the bounds if necessary.
#define CLAMP(val, lower, upper)                                                                   \
    (({                                                                                            \
        __typeof__(val)   _val   = (val);                                                          \
        __typeof__(lower) _lower = (lower);                                                        \
        __typeof__(upper) _upper = (upper);                                                        \
        (MIN(MAX(_val, _lower), _upper));                                                          \
    }))

extern thread_local std::mt19937                     randGen;  // Defined in render.cpp
extern thread_local std::uniform_real_distribution<> randDist; // Uniform [-1.0, 1.0) distribution,
                                                               // defined in render.cpp

/**
 * @brief Returns a random float in range [0, 1)
 *
 * @return float
 */
static inline float randomDouble()
{
    return (randDist(randGen) + 1.0) / 2.0;
}
