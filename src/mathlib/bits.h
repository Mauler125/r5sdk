#ifndef MATHLIB_BITS_H
#define MATHLIB_BITS_H
//=============================================================================//
//
// Purpose: bitwise utilities.
//
//=============================================================================//

//-----------------------------------------------------------------------------
// '__popcnt' instruction reimplementation (allows for usage without requiring
// a processor to feature the popcnt instruction, only use for tools that have
// to run on processors that do NOT support popcnt!).
//-----------------------------------------------------------------------------
inline unsigned int PopCount(unsigned long long x)
{
    unsigned int count = 0;
    while (x) {
        x &= x - 1; // Clears the least significant bit set to 1.
        count++;
    }
    return count;
}

#endif // MATHLIB_BITS_H
