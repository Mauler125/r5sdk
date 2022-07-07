//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "core/stdafx.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Purpose: qt = ( s * p ) * q
//-----------------------------------------------------------------------------
void QuaternionSM(float s, const Quaternion& p, const Quaternion& q, Quaternion& qt)
{
	Quaternion		p1, q1;

	QuaternionScale(p, s, p1);
	QuaternionMult(p1, q, q1);
	QuaternionNormalize(q1);
	qt[0] = q1[0];
	qt[1] = q1[1];
	qt[2] = q1[2];
	qt[3] = q1[3];
}

#if ALLOW_SIMD_QUATERNION_MATH
FORCEINLINE fltx4 QuaternionSMSIMD(const fltx4& s, const fltx4& p, const fltx4& q)
{
	fltx4 p1, q1, result;
	p1 = QuaternionScaleSIMD(p, s);
	q1 = QuaternionMultSIMD(p1, q);
	result = QuaternionNormalizeSIMD(q1);
	return result;
}

FORCEINLINE fltx4 QuaternionSMSIMD(float s, const fltx4& p, const fltx4& q)
{
	return QuaternionSMSIMD(ReplicateX4(s), p, q);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: qt = p * ( s * q )
//-----------------------------------------------------------------------------
void QuaternionMA(const Quaternion& p, float s, const Quaternion& q, Quaternion& qt)
{
	Quaternion p1, q1;

	QuaternionScale(q, s, q1);
	QuaternionMult(p, q1, p1);
	QuaternionNormalize(p1);
	qt[0] = p1[0];
	qt[1] = p1[1];
	qt[2] = p1[2];
	qt[3] = p1[3];
}

#if ALLOW_SIMD_QUATERNION_MATH

FORCEINLINE fltx4 QuaternionMASIMD(const fltx4& p, const fltx4& s, const fltx4& q)
{
	fltx4 p1, q1, result;
	q1 = QuaternionScaleSIMD(q, s);
	p1 = QuaternionMultSIMD(p, q1);
	result = QuaternionNormalizeSIMD(p1);
	return result;
}

FORCEINLINE fltx4 QuaternionMASIMD(const fltx4& p, float s, const fltx4& q)
{
	return QuaternionMASIMD(p, ReplicateX4(s), q);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: qt = p + s * q
//-----------------------------------------------------------------------------
void QuaternionAccumulate(const Quaternion& p, float s, const Quaternion& q, Quaternion& qt)
{
	Quaternion q2;
	QuaternionAlign(p, q, q2);

	qt[0] = p[0] + s * q2[0];
	qt[1] = p[1] + s * q2[1];
	qt[2] = p[2] + s * q2[2];
	qt[3] = p[3] + s * q2[3];
}

#if ALLOW_SIMD_QUATERNION_MATH
FORCEINLINE fltx4 QuaternionAccumulateSIMD(const fltx4& p, float s, const fltx4& q)
{
	fltx4 q2, s4, result;
	q2 = QuaternionAlignSIMD(p, q);
	s4 = ReplicateX4(s);
	result = MaddSIMD(s4, q2, p);
	return result;
}
#endif
