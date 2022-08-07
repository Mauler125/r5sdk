//====== Copyright 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef TRANSFORM_H
#define TRANSFORM_H

#ifdef COMPILER_MSVC
#pragma once
#endif

//#include "tier0/memalloc.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Matrix 3x4_t
//-----------------------------------------------------------------------------
class CTransformUnaligned;


//-----------------------------------------------------------------------------
// Represents a position + orientation using quaternions
//-----------------------------------------------------------------------------
class ALIGN16 CTransform
{
public:
	CTransform() {}
	CTransform(const Vector3D& v, const Quaternion& q) : m_vPosition(v), m_orientation(q) {}
	CTransform(const Vector3D& v, const QAngle& a) : m_vPosition(v)
	{
		AngleQuaternion(a, m_orientation);
	}

	VectorAligned m_vPosition;
	QuaternionAligned m_orientation;

	bool IsValid() const
	{
		return m_vPosition.IsValid() && m_orientation.IsValid();
	}

	bool operator==(const CTransform& v) const;					///< exact equality check
	bool operator!=(const CTransform& v) const;

	// for API compatibility with matrix3x4_t
	inline void InitFromQAngles(const QAngle& angles, const Vector3D& vPosition = vec3_origin);
	inline void InitFromMatrix(const matrix3x4_t& transform);
	inline void InitFromQuaternion(const Quaternion& orientation, const Vector3D& vPosition = vec3_origin);

	inline Quaternion ToQuaternion() const;
	inline QAngle ToQAngle() const;
	inline matrix3x4_t ToMatrix() const;

	inline void SetToIdentity();

	inline void SetOrigin(Vector3D const& vPos) { m_vPosition = vPos; }
	inline void SetAngles(QAngle const& vAngles);
	inline Vector3D GetOrigin(void) const { return m_vPosition; }

	inline void GetBasisVectorsFLU(Vector3D* pForward, Vector3D* pLeft, Vector3D* pUp) const;
	inline Vector3D GetForward() const;
	inline Vector3D TransformVector(const Vector3D& v0) const;
	inline Vector3D RotateVector(const Vector3D& v0) const;
	inline Vector3D TransformVectorByInverse(const Vector3D& v0) const;
	inline Vector3D RotateVectorByInverse(const Vector3D& v0) const;
	inline Vector3D RotateExtents(const Vector3D& vBoxExtents) const; // these are extents and must remain positive/symmetric after rotation
	inline void TransformAABB(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const;
	inline void TransformAABBByInverse(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const;
	inline void RotateAABB(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const;
	inline void RotateAABBByInverse(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const;
	//inline void TransformPlane( const cplane_t &inPlane, cplane_t &outPlane ) const;
	//inline void InverseTransformPlane( const cplane_t &inPlane, cplane_t &outPlane ) const;

	/// Computes an inverse.  Uses the 'TR' naming to be consistent with the same method in matrix3x4_t (which only works with orthonormal matrices) 
	inline void InverseTR(CTransform& out) const;

public:
	CTransform& operator=(const CTransformUnaligned& i);
} ALIGN16_POST;


extern const CTransform g_TransformIdentity;


//-----------------------------------------------------------------------------
// Represents an unaligned position + orientation using quaternions,
// used only for copying data around
//-----------------------------------------------------------------------------
class CTransformUnaligned
{
public:
	CTransformUnaligned() {}
	CTransformUnaligned(const Vector3D& v, const Quaternion& q) : m_vPosition(v), m_orientation(q) {}
	CTransformUnaligned(const CTransform& transform) : m_vPosition(transform.m_vPosition), m_orientation(transform.m_orientation) {}
	CTransform AsTransform() const { return CTransform(m_vPosition, m_orientation); }

	Vector3D m_vPosition;
	Quaternion m_orientation;

	bool IsValid() const
	{
		return m_vPosition.IsValid() && m_orientation.IsValid();
	}

public:
	CTransformUnaligned& operator=(const CTransform& i);
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CTransform& CTransform::operator=(const CTransformUnaligned& i)
{
	m_vPosition = i.m_vPosition;
	m_orientation = i.m_orientation;
	return *this;
}

inline CTransformUnaligned& CTransformUnaligned::operator=(const CTransform& i)
{
	m_vPosition = i.m_vPosition;
	m_orientation = i.m_orientation;
	return *this;
}


//-----------------------------------------------------------------------------
// Other methods
//-----------------------------------------------------------------------------
void ConcatTransforms(const CTransform& in1, const CTransform& in2, CTransform& out);
void TransformSlerp(const CTransform& p, const CTransform& q, float t, CTransform& qt);
void TransformLerp(const CTransform& p, const CTransform& q, float t, CTransform& qt);
void TransformMatrix(const CTransform& in, matrix3x4_t& out);
void TransformMatrix(const CTransform& in, const Vector3D& vScaleIn, matrix3x4_t& out);

inline void TransformMatrix(const CTransform& in, float flScale, matrix3x4_t& out)
{
	QuaternionMatrix(in.m_orientation, in.m_vPosition, Vector3D(flScale, flScale, flScale), out);
}

inline float TransformNormalize(CTransform& in)
{
	return QuaternionNormalize(in.m_orientation);
}

void TransformMatrix(const CTransformUnaligned& in, matrix3x4_t& out);
void MatrixTransform(const matrix3x4_t& in, CTransform& out);
void MatrixTransform(const matrix3x4_t& in, CTransformUnaligned& out);
void MatrixTransform(const matrix3x4_t& in, CTransform& out, Vector3D& vScaleOut);

inline void MatrixTransform(const matrix3x4_t& in, CTransform& out, float& flScale)
{
	Vector3D vScale;
	MatrixTransform(in, out, vScale);
	flScale = vScale.LargestComponentValue();
}

void AngleTransform(const QAngle& angles, const Vector3D& origin, CTransform& out);
void SetIdentityTransform(CTransform& out);
void TransformVectorsFLU(const CTransform& in, Vector3D* pForward, Vector3D* pLeft, Vector3D* pUp);
void TransformVectorsForward(const CTransform& in, Vector3D* pForward);

inline const CTransform GetIdentityTransform()
{
	CTransform out;
	SetIdentityTransform(out);
	return out;
}

inline const CTransform MatrixTransform(const matrix3x4_t& in)
{
	CTransform out;
	MatrixTransform(in, out);
	return out;
}

inline const matrix3x4_t TransformMatrix(const CTransform& in)
{
	matrix3x4_t out;
	TransformMatrix(in, out);
	return out;
}
inline const matrix3x4_t TransformMatrix(const CTransformUnaligned& in)
{
	matrix3x4_t out;
	TransformMatrix(in, out);
	return out;
}

inline const CTransform ConcatTransforms(const CTransform& in1, const CTransform& in2)
{
	CTransform result;
	ConcatTransforms(in1, in2, result);
	return result;
}


void TransformInvert(const CTransform& in, CTransform& out);
void AxisAngleTransform(const Vector3D& vecAxis, float flAngleDegrees, CTransform& out);
void VectorIRotate(const Vector3D& v, const CTransform& t, Vector3D& out);
void VectorITransform(const Vector3D& v, const CTransform& t, Vector3D& out);

inline Vector3D TransformPoint(const CTransformUnaligned& tm, const Vector3D& p)
{
	return Vector3D(
		tm.m_vPosition.x + (1.0f - 2.0f * tm.m_orientation.y * tm.m_orientation.y - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.x + (2.0f * tm.m_orientation.x * tm.m_orientation.y - 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.x * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.z,
		tm.m_vPosition.y + (2.0f * tm.m_orientation.x * tm.m_orientation.y + 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.x + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.y * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.z,
		tm.m_vPosition.z + (2.0f * tm.m_orientation.x * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.x + (2.0f * tm.m_orientation.y * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.y + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.y * tm.m_orientation.y) * p.z
	);
}

// TODO: implement in SIMD?
inline Vector3D TransformPoint(const CTransform& tm, const Vector3D& p)
{
	return Vector3D(
		tm.m_vPosition.x + (1.0f - 2.0f * tm.m_orientation.y * tm.m_orientation.y - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.x + (2.0f * tm.m_orientation.x * tm.m_orientation.y - 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.x * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.z,
		tm.m_vPosition.y + (2.0f * tm.m_orientation.x * tm.m_orientation.y + 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.x + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.y * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.z,
		tm.m_vPosition.z + (2.0f * tm.m_orientation.x * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.x + (2.0f * tm.m_orientation.y * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.y + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.y * tm.m_orientation.y) * p.z
	);
}


template < class T >
inline void TransformPoint(const T& tm, const Vector3D& p, Vector3D& out)
{
	out.x = tm.m_vPosition.x + (1.0f - 2.0f * tm.m_orientation.y * tm.m_orientation.y - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.x + (2.0f * tm.m_orientation.x * tm.m_orientation.y - 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.x * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.z;
	out.y = tm.m_vPosition.y + (2.0f * tm.m_orientation.x * tm.m_orientation.y + 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.x + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.y * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.z;
	out.z = tm.m_vPosition.z + (2.0f * tm.m_orientation.x * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.x + (2.0f * tm.m_orientation.y * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.y + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.y * tm.m_orientation.y) * p.z;
}

template < class T >
inline void RotatePoint(const T& tm, const Vector3D& p, Vector3D& out)
{
	out.x = (1.0f - 2.0f * tm.m_orientation.y * tm.m_orientation.y - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.x + (2.0f * tm.m_orientation.x * tm.m_orientation.y - 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.x * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.z;
	out.y = (2.0f * tm.m_orientation.x * tm.m_orientation.y + 2.0f * tm.m_orientation.w * tm.m_orientation.z) * p.x + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.z * tm.m_orientation.z) * p.y + (2.0f * tm.m_orientation.y * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.z;
	out.z = (2.0f * tm.m_orientation.x * tm.m_orientation.z - 2.0f * tm.m_orientation.w * tm.m_orientation.y) * p.x + (2.0f * tm.m_orientation.y * tm.m_orientation.z + 2.0f * tm.m_orientation.w * tm.m_orientation.x) * p.y + (1.0f - 2.0f * tm.m_orientation.x * tm.m_orientation.x - 2.0f * tm.m_orientation.y * tm.m_orientation.y) * p.z;
}


inline const CTransform TransformInvert(const CTransform& in)
{
	CTransform out;
	TransformInvert(in, out);
	return out;
}

// Transform equality test
bool TransformsAreEqual(const CTransform& src1, const CTransform& src2, float flPosTolerance = 1e-2, float flRotTolerance = 1e-1f);

// Computes world-space transforms given local-space transforms + parent info
// The start of the pTransforms array (nRootTransformCount # of transforms) must be filled with 
// the root transforms which have no parent. The end of the pTransforms array (nTransformCount # of transforms)
// must be filled with local-space transforms which are relative to other transforms, including possibly the
// root transforms. Therefore, (nRootTransformCount + nTransformCount) # of transforms must be passed into pTransforms.
// Only nTransformCount parent indices should be passed in. 
// Parent indices are relative to the entire array, so a parent index of 0 indicates the first element
// of the array, which is always a root transform. -1 parent index is *illegal*
// Parent indices must always be sorted so that the index transforms earlier in the array.
// The transforms are modified in-place.
void TransformToWorldSpace(int nRootTransformCount, int nTransformCount, const int* pParentIndices, CTransform* pTransforms);
void TransformToParentSpace(int nRootTransformCount, int nTransformCount, const int* pParentIndices, CTransform* pTransforms);


inline void CTransform::InitFromQAngles(const QAngle& angles, const Vector3D& vPosition)
{
	AngleQuaternion(angles, m_orientation);
	m_vPosition = vPosition;
}

inline void CTransform::InitFromMatrix(const matrix3x4_t& transform)
{
	m_orientation = MatrixQuaternion(transform);
	m_vPosition = transform.GetOrigin();
}

inline void CTransform::InitFromQuaternion(const Quaternion& orientation, const Vector3D& vPosition)
{
	m_orientation = orientation;
	m_vPosition = vPosition;
}

inline void CTransform::SetAngles(QAngle const& vAngles)
{
	AngleQuaternion(vAngles, m_orientation);
}

inline Quaternion CTransform::ToQuaternion() const
{
	return m_orientation;
}
inline QAngle CTransform::ToQAngle() const
{
	QAngle angles;
	QuaternionAngles(m_orientation, angles);
	return angles;
}

inline matrix3x4_t CTransform::ToMatrix() const
{
	return TransformMatrix(*this);
}

inline void CTransform::SetToIdentity()
{
	m_vPosition = vec3_origin;
	m_orientation = quat_identity;
}

inline void CTransform::GetBasisVectorsFLU(Vector3D* pForward, Vector3D* pLeft, Vector3D* pUp) const
{
	TransformVectorsFLU(*this, pForward, pLeft, pUp);
}

inline Vector3D CTransform::GetForward() const
{
	Vector3D vForward;
	TransformVectorsForward(*this, &vForward);
	return vForward;
}

inline Vector3D CTransform::TransformVector(const Vector3D& v0) const
{
	return TransformPoint(*this, v0);
}

inline Vector3D CTransform::RotateVector(const Vector3D& v0) const
{
	Vector3D vOut;
	RotatePoint(*this, v0, vOut);
	return vOut;
}

inline Vector3D CTransform::TransformVectorByInverse(const Vector3D& v0) const
{
	Vector3D vOut;
	VectorITransform(v0, *this, vOut);
	return vOut;
}

inline Vector3D CTransform::RotateVectorByInverse(const Vector3D& v0) const
{
	Vector3D vOut;
	VectorIRotate(v0, *this, vOut);
	return vOut;
}

inline bool CTransform::operator==(const CTransform& t) const
{
	return t.m_vPosition == m_vPosition && t.m_orientation == m_orientation;
}

inline bool CTransform::operator!=(const CTransform& t) const
{
	return t.m_vPosition != m_vPosition || t.m_orientation != m_orientation;
}

// PERFORMANCE: No native versions of these but implement them on matrix for convenient access
inline void CTransform::TransformAABB(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const
{
	ToMatrix().TransformAABB(vecMinsIn, vecMaxsIn, vecMinsOut, vecMaxsOut);
}

inline void CTransform::TransformAABBByInverse(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const
{
	ToMatrix().TransformAABBByInverse(vecMinsIn, vecMaxsIn, vecMinsOut, vecMaxsOut);
}

inline void CTransform::RotateAABB(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const
{
	ToMatrix().RotateAABB(vecMinsIn, vecMaxsIn, vecMinsOut, vecMaxsOut);
}
inline void CTransform::RotateAABBByInverse(const Vector3D& vecMinsIn, const Vector3D& vecMaxsIn, Vector3D& vecMinsOut, Vector3D& vecMaxsOut) const
{
	ToMatrix().RotateAABBByInverse(vecMinsIn, vecMaxsIn, vecMinsOut, vecMaxsOut);
}

inline void CTransform::InverseTR(CTransform& out) const
{
	matrix3x4_t xForm = ToMatrix();
	out = xForm.InverseTR().ToCTransform();
}


// transform conversion operators on matrix3x4_t
inline void matrix3x4_t::InitFromCTransform(const CTransform& transform)
{
	TransformMatrix(transform, *this);
}
inline CTransform matrix3x4_t::ToCTransform() const
{
	return MatrixTransform(*this);
}


#endif // TRANSFORM