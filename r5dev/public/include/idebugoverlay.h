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
	void AddBoxOverlay(__m128i& vTransforms, const Vector3D& vMins, const Vector3D& vMaxs, int r, int g, int b, int a, bool bZBuffer, float flDuration)
	{
		const int index = 1;
		CallVFunc<void>(index, this, vTransforms, vMins, vMaxs, r, g, b, a, bZBuffer, flDuration);
	}
	void AddSphereOverlay(const Vector3D& vOrigin, float flRadius, int nTheta, int nPhi, int r, int g, int b, int a, float flDuration)
	{
		const int index = 3;
		CallVFunc<void>(index, this, vOrigin, flRadius, nTheta, nPhi, r, g, b, a, flDuration);
	}
	void AddLineOverlay(const Vector3D& vStart, const Vector3D& vEnd, int r, int g, int b, char bZBuffer, float flDuration)
	{
		const int index = 5;
		CallVFunc<void>(index, this, vStart, vEnd, r, g, b, bZBuffer, flDuration);
	}
	void AddCapsuleOverlay(const Vector3D& vStart, const Vector3D& vEnd, const Vector3D& vRadius, const Vector3D& vTop, const Vector3D& vBottom, int r, int g, int b, int a, float flDuration)
	{
		const int index = 12;
		CallVFunc<void>(index, this, vStart, vEnd, vRadius, vTop, vBottom, r, g, b, a, flDuration);
	}
};

inline CIVDebugOverlay* g_pDebugOverlay = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VDebugOverlayBase : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pDebugOverlay                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pDebugOverlay));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pDebugOverlay = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8D\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28\xF3\x0F\x10\x41\x00"), "xxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxxxx?")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<CIVDebugOverlay*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VDebugOverlayBase);
#endif // IDEBUGOVERLAY_H