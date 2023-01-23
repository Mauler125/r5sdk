//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Debug overlay engine interface.
//
//===========================================================================//
//
///////////////////////////////////////////////////////////////////////////////
#ifndef IDEBUGOVERLAY_H
#define IDEBUGOVERLAY_H

#include "mathlib/vector.h"
class IVDebugOverlay
{
	void* __vftable /*VFT*/;
};

class CIVDebugOverlay : public IVDebugOverlay
{
public:
	void AddBoxOverlay(matrix3x4_t& vTransforms, const Vector3D& vMins, const Vector3D& vMaxs, int r, int g, int b, int a, bool bZBuffer, float flDuration)
	{
		const static int index = 1;
		CallVFunc<void>(index, this, vTransforms, vMins, vMaxs, r, g, b, a, bZBuffer, flDuration);
	}
	void AddSphereOverlay(const Vector3D& vOrigin, float flRadius, int nTheta, int nPhi, int r, int g, int b, int a, float flDuration)
	{
		const static int index = 3;
		CallVFunc<void>(index, this, vOrigin, flRadius, nTheta, nPhi, r, g, b, a, flDuration);
	}
	void AddLineOverlay(const Vector3D& vStart, const Vector3D& vEnd, int r, int g, int b, char noDepthTest, float flDuration)
	{
		const static int index = 5;
		CallVFunc<void>(index, this, vStart, vEnd, r, g, b, noDepthTest, flDuration);
	}
	void AddCapsuleOverlay(const Vector3D& vStart, const Vector3D& vEnd, const Vector3D& vRadius, const Vector3D& vTop, const Vector3D& vBottom, int r, int g, int b, int a, float flDuration)
	{
		const static int index = 12;
		CallVFunc<void>(index, this, vStart, vEnd, vRadius, vTop, vBottom, r, g, b, a, flDuration);
	}
};

inline CIVDebugOverlay* g_pDebugOverlay = nullptr;
#endif // IDEBUGOVERLAY_H