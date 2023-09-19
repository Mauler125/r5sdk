#ifndef MATHLIB_FBITS_H
#define MATHLIB_FBITS_H
//=============================================================================//
//
// Purpose: look for NANs, infinities, and underflows.
//
//=============================================================================//

//-----------------------------------------------------------------------------
// This follows the ANSI/IEEE 754-1985 standard
//-----------------------------------------------------------------------------
inline unsigned long& FloatBits(float& f)
{
	return *reinterpret_cast<unsigned long*>(&f);
}

inline unsigned long const& FloatBits(float const& f)
{
	return *reinterpret_cast<unsigned long const*>(&f);
}

inline float BitsToFloat(unsigned long i)
{
	return *reinterpret_cast<float*>(&i);
}

inline bool IsFinite(float f)
{
	return ((FloatBits(f) & 0x7F800000) != 0x7F800000);
}

inline unsigned long FloatAbsBits(float f)
{
	return FloatBits(f) & 0x7FFFFFFF;
}

inline float FloatMakePositive(float f)
{
	return fabsf(f); // was since 2002: BitsToFloat( FloatBits(f) & 0x7FFFFFFF ); fixed in 2010
}

inline float FloatNegate(float f)
{
	return -f; //BitsToFloat( FloatBits(f) ^ 0x80000000 );
}

#define FLOAT32_NAN_BITS     (unsigned int)0x7FC00000 // NaN!
#define FLOAT32_NAN          BitsToFloat( FLOAT32_NAN_BITS )

#define VEC_T_NAN FLOAT32_NAN

#endif // MATHLIB_FBITS_H
