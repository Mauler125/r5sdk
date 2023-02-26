//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "core/stdafx.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Purpose: Calculate the volume of a tetrahedron with these vertices
// Input  : p0 - points of tetrahedron
//			p1 - 
//			p2 - 
//			p3 - 
// Output : float (volume in units^3)
//-----------------------------------------------------------------------------
static float TetrahedronVolume(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3)
{
	Vector3D a, b, c, cross;
	float volume = 1.0f / 6.0f;

	a = p1 - p0;
	b = p2 - p0;
	c = p3 - p0;
	cross = CrossProduct(b, c);

	volume *= DotProduct(a, cross);
	if (volume < 0)
		return -volume;
	return volume;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate a triangle area with these vertices
// Input  : p0 - points of tetrahedron
//			p1 - 
//			p2 - 
// Output : float (area)
//-----------------------------------------------------------------------------
static float TriangleArea(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2)
{
	Vector3D e0 = p1 - p0;
	Vector3D e1 = p2 - p0;
	Vector3D cross;

	CrossProduct(e0, e1, cross);
	return 0.5f * cross.Length();
}
