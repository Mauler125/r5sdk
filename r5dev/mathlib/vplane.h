//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef VPLANE_H
#define VPLANE_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

typedef int SideType;

// Used to represent sides of things like planes.
#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define VP_EPSILON	0.01f

class VPlane
{
public:
	VPlane();
	VPlane(const Vector3D& vNormal, vec_t dist);
	VPlane(const Vector3D& vPoint, const QAngle& ang);

	void		Init(const Vector3D& vNormal, vec_t dist);

	// Return the distance from the point to the plane.
	vec_t		DistTo(const Vector3D& vVec) const;

	// Copy.
	VPlane& operator=(const VPlane& thePlane);

	// Returns SIDE_ON, SIDE_FRONT, or SIDE_BACK.
	// The epsilon for SIDE_ON can be passed in.
	SideType	GetPointSide(const Vector3D& vPoint, vec_t sideEpsilon = VP_EPSILON) const;

	// Returns SIDE_FRONT or SIDE_BACK.
	SideType	GetPointSideExact(const Vector3D& vPoint) const;

	// Classify the box with respect to the plane.
	// Returns SIDE_ON, SIDE_FRONT, or SIDE_BACK
	SideType	BoxOnPlaneSide(const Vector3D& vMin, const Vector3D& vMax) const;

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// Flip the plane.
	VPlane		Flip();

	// Get a point on the plane (normal*dist).
	Vector3D		GetPointOnPlane() const;

	// Snap the specified point to the plane (along the plane's normal).
	Vector3D		SnapPointToPlane(const Vector3D& vPoint) const;
#endif

public:
	Vector3D		m_Normal;
	vec_t		m_Dist;

#ifdef VECTOR_NO_SLOW_OPERATIONS
private:
	// No copy constructors allowed if we're in optimal mode
	VPlane(const VPlane& vOther);
#endif
};


//-----------------------------------------------------------------------------
// Inlines.
//-----------------------------------------------------------------------------
inline VPlane::VPlane()
{
}

inline VPlane::VPlane(const Vector3D& vNormal, vec_t dist)
{
	m_Normal = vNormal;
	m_Dist = dist;
}

inline VPlane::VPlane(const Vector3D& vPoint, const QAngle& ang)
{
	m_Normal = ang.GetNormal();
	m_Dist = vPoint.x * m_Normal.x + vPoint.y * m_Normal.y + vPoint.z * m_Normal.z;
}

inline void	VPlane::Init(const Vector3D& vNormal, vec_t dist)
{
	m_Normal = vNormal;
	m_Dist = dist;
}

inline vec_t VPlane::DistTo(const Vector3D& vVec) const
{
	return vVec.Dot(m_Normal) - m_Dist;
}

inline VPlane& VPlane::operator=(const VPlane& thePlane)
{
	m_Normal = thePlane.m_Normal;
	m_Dist = thePlane.m_Dist;
	return *this;
}

#ifndef VECTOR_NO_SLOW_OPERATIONS

inline VPlane VPlane::Flip()
{
	return VPlane(-m_Normal, -m_Dist);
}

inline Vector3D VPlane::GetPointOnPlane() const
{
	return m_Normal * m_Dist;
}

inline Vector3D VPlane::SnapPointToPlane(const Vector3D& vPoint) const
{
	return vPoint - m_Normal * DistTo(vPoint);
}

#endif

inline SideType VPlane::GetPointSide(const Vector3D& vPoint, vec_t sideEpsilon) const
{
	vec_t fDist;

	fDist = DistTo(vPoint);
	if (fDist >= sideEpsilon)
		return SIDE_FRONT;
	else if (fDist <= -sideEpsilon)
		return SIDE_BACK;
	else
		return SIDE_ON;
}

inline SideType VPlane::GetPointSideExact(const Vector3D& vPoint) const
{
	return DistTo(vPoint) > 0.0f ? SIDE_FRONT : SIDE_BACK;
}


// BUGBUG: This should either simply use the implementation in mathlib or cease to exist.
// mathlib implementation is much more efficient.  Check to see that VPlane isn't used in
// performance critical code.
inline SideType VPlane::BoxOnPlaneSide(const Vector3D& vMin, const Vector3D& vMax) const
{
	int i, firstSide, side;
	TableVector vPoints[8] =
	{
		{ vMin.x, vMin.y, vMin.z },
		{ vMin.x, vMin.y, vMax.z },
		{ vMin.x, vMax.y, vMax.z },
		{ vMin.x, vMax.y, vMin.z },

		{ vMax.x, vMin.y, vMin.z },
		{ vMax.x, vMin.y, vMax.z },
		{ vMax.x, vMax.y, vMax.z },
		{ vMax.x, vMax.y, vMin.z },
	};

	firstSide = GetPointSideExact(vPoints[0]);
	for (i = 1; i < 8; i++)
	{
		side = GetPointSideExact(vPoints[i]);

		// Does the box cross the plane?
		if (side != firstSide)
			return SIDE_ON;
	}

	// Ok, they're all on the same side, return that.
	return firstSide;
}

#endif // VPLANE_H
