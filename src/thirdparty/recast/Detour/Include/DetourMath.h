/**
@defgroup detour Detour

Members in this module are wrappers around the standard math library
*/

#ifndef DETOURMATH_H
#define DETOURMATH_H

#include <math.h>

/// The value of PI used by Recast.
static const float DT_PI = 3.14159265f;

/// The total number of bits in an bit cell integer.
static const int DT_BITS_PER_BIT_CELL = 32;

inline int dtBitCellBit(const int bitNum) { return (1 << ((bitNum) & (DT_BITS_PER_BIT_CELL-1))); }

inline float dtMathFabsf(float x) { return fabsf(x); }
inline float dtMathSqrtf(float x) { return sqrtf(x); }
inline float dtMathFloorf(float x) { return floorf(x); }
inline float dtMathCeilf(float x) { return ceilf(x); }
inline float dtMathRoundf(float x) { return roundf(x); }
inline float dtMathCosf(float x) { return cosf(x); }
inline float dtMathSinf(float x) { return sinf(x); }
inline float dtMathAtan2f(float y, float x) { return atan2f(y, x); }
inline bool dtMathIsfinite(float x) { return isfinite(x); }

#endif
